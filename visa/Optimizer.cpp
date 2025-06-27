/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Optimizer.h"
#include "Assertions.h"
#include "G4_Opcode.h"
#include "G4_Verifier.hpp"
#include "Timer.h"
#include "ifcvt.h"
#include "Common_BinaryEncoding.h"
#include "DebugInfo.h"
#include "FlowGraph.h"
#include "Passes/AccSubstitution.hpp"
#include "PointsToAnalysis.h"
#include "Passes/SRSubstitution.hpp"
#include "Passes/InstCombine.hpp"
#include "Passes/LVN.hpp"
#include "Passes/MergeScalars.hpp"
#include "Passes/SendFusion.hpp"
#include "Passes/StaticProfiling.hpp"

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/Allocator.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include <optional>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <map>
#include <random>
#include <sstream>
#include <tuple>
#include <vector>

using namespace vISA;

void Optimizer::LVN() {
  // Run a simple LVN pass that replaces redundant
  // immediate loads in current BB. Also this pass
  // does not optimize operations like a
  // conventional VN pass because those require
  // more compile time, and are presumably already
  // done by FE generating VISA. This pass catches
  // redundancies that got introduced mainly by HW
  // conformity or due to VISA lowering.
  int numInstsRemoved = 0;
  PointsToAnalysis p(kernel.Declares, kernel.fg.getNumBB());
  p.doPointsToAnalysis(kernel.fg);
  for (auto bb : kernel.fg) {
    ::LVN lvn(fg, bb, *fg.builder, p);
    lvn.doLVN();
    numInstsRemoved += lvn.getNumInstsRemoved();
    numInstsRemoved += ::LVN::removeRedundantSamplerMovs(kernel, bb);
  }

  VISA_DEBUG({
    std::cout << "===== LVN ====="
              << "\n";
    std::cout << "Number of instructions removed: " << numInstsRemoved << "\n"
              << "\n";
  });
}

// helper functions

static int getDstSubReg(G4_DstRegRegion *dst) {
  int dstSubReg;
  if (dst->getBase()->isPhyReg()) {
    dstSubReg = dst->getSubRegOff();
  } else {
    dstSubReg = dst->getSubRegOff() +
                static_cast<G4_RegVar *>(dst->getBase())->getPhyRegOff();
  }

  return dstSubReg;
}

static int getSrcSubReg(G4_Operand *src) {
  vISA_ASSERT(src->isSrcRegRegion(), "expect Src Reg Region");
  int srcSubReg;
  if (src->asSrcRegRegion()->getBase()->isPhyReg()) {
    srcSubReg = src->asSrcRegRegion()->getSubRegOff();
  } else {
    srcSubReg = src->asSrcRegRegion()->getSubRegOff() +
                static_cast<G4_RegVar *>(src->asSrcRegRegion()->getBase())
                    ->getPhyRegOff();
  }
  return srcSubReg;
}

//
// determine if fall-through jump is needed
//
// also remove redundant jumps
//   if there is no predicate applied to a jump and its target is its fall
//   though BB, remove the jump instruction.

void Optimizer::insertFallThroughJump() {

  fg.setPhysicalPredSucc();
  for (BB_LIST_ITER it = fg.begin(); it != fg.end();) {
    G4_BB *bb = *it;
    BB_LIST_ITER next = ++it;
    //
    // determine if the current bb needs a fall through jump
    // check if the fall-through bb follows the current bb
    //
    G4_BB *fb = bb->fallThroughBB();
    if (fb && (next == fg.end() || // bb is the last bb
               fb != (*next))) {
      // This is bogus in SIMD CF, as bad things happen when you randomly insert
      // jumps in the middle of SIMD CF
    } else if (next != fg.end()) {
      // do not remove a jmpi if it's the target of an indirect jmp
      // this makes the code more readable
      if (!(*next)->empty() && (*next)->front()->isLabel() && !bb->empty() &&
          bb->back()->opcode() == G4_jmpi &&
          bb->back()->getPredicate() == NULL &&
          !fg.isIndirectJmpTarget(bb->back())) {
        if ((*next)->front()->getSrc(0) == bb->back()->getSrc(0)) {
          std::list<G4_INST *>::iterator it = bb->end();
          it--;
          bb->erase(it);
        }
      }
    }
    it = next;
  }
}

void Optimizer::forceAssignRegs() {
  const char *rawStr =
      builder.getOptions()->getOptionCstr(vISA_ForceAssignRhysicalReg);
  if (!rawStr)
    return;

  llvm::StringRef line(rawStr);
  llvm::SmallVector<llvm::StringRef, 4> assignments;
  line.split(assignments, ',');
  std::map<std::string /*decl name or id*/,
           std::pair<int /*reg*/, int /*subreg*/>> forceAssign;
  for (llvm::StringRef assignment : assignments) {
    llvm::StringRef decl, reg, subreg;
    std::tie(decl, reg) = assignment.split(':');
    std::tie(reg, subreg) = reg.split('.');
    int regNum = std::stoi(reg.str());
    int subregNum = subreg.empty() ? 0 : std::stoi(subreg.str());
    forceAssign[decl.str()] = std::make_pair(regNum, subregNum);
  }

  for (G4_Declare *dcl : kernel.Declares) {
    int reg, subreg;
    // skip forcing register assignment for the decl if both name and id are
    // not specified in the option. Name will be used if both are given.
    auto it = forceAssign.find(dcl->getName());
    if (it == forceAssign.end()) {
      it = forceAssign.find(std::to_string(dcl->getDeclId()));
      if (it == forceAssign.end())
        continue;
    }
    std::tie(reg, subreg) = it->second;
    dcl->getRegVar()->setPhyReg(builder.phyregpool.getGreg(reg), subreg);
    VISA_DEBUG({
        std::cerr << "Force assigning Decl : " << it->first
                  << " to r" << reg << "." << subreg << "\n";
        dcl->dump();
    });
  }
}

void Optimizer::forceSpillVars() {
  const char *rawStr =
      builder.getOptions()->getOptionCstr(vISA_ForceSpillVariables);
  if (!rawStr)
     return;

  llvm::StringRef line(rawStr);
  llvm::SmallVector<llvm::StringRef, 4> vars;
  line.split(vars, ',');
  std::vector<int> token;

  for (llvm::StringRef var : vars)
    token.push_back(std::stoi(var.str()));

  for (G4_Declare *dcl : kernel.Declares) {
    if (std::find(token.begin(), token.end(), dcl->getDeclId()) !=
        token.end()) {
      dcl->setForceSpilled();
    }
  }
}

void Optimizer::preRegAlloc() {
  forceAssignRegs();
  forceSpillVars();
}

void Optimizer::regAlloc() {

  fg.prepareTraversal();

  // realR0 and BuiltInR0 are 2 different dcls.
  // realR0 is always tied to physical r0.
  // if copy of r0 isnt needed then set latter to r0 as well.
  // if copy of r0 is required, then let RA decide allocation of BuiltInR0.
  if (!R0CopyNeeded()) {
    // when no copy is needed, make BuiltInR0 an alias of realR0
    builder.getBuiltinR0()->setAliasDeclare(builder.getRealR0(), 0);
    builder.getBuiltinR0()->getRegVar()->setPhyReg(
        builder.getRealR0()->getRegVar()->getPhyReg(), 0);
  }

  //
  // assign registers
  //
  int status = ::regAlloc(builder, builder.phyregpool, kernel);
  if (status == VISA_EARLY_EXIT) {
    EarlyExited = true;
  } else if (status != VISA_SUCCESS) {
    RAFail = true;
  }
}

// HW debugging needs to zero certain ARF registers such as a0, acc, etc.
// Here, we zero a0 and acc on entry to a kernel.
void Optimizer::zeroSomeARF() {
  if (builder.getIsKernel()) {
    // The first BB is not necessarily the kernel's entry when kernel needs to
    // load its payload!
    G4_BB *mainBB = fg.getEntryBB();
    if (builder.loadThreadPayload()) {
      // Make sure to skip prolog BBs to insert into the 1st BB of a kernel.
      //   [perThreadBB:]
      //   crossThreadBB:
      //   main:
      if (G4_BB *crossThreadBB = kernel.getCrossThreadPayloadBB()) {
        vASSERT(crossThreadBB->Succs.size() == 1);
        mainBB = crossThreadBB->Succs.front();
      } else if (G4_BB *perThreadBB = kernel.getPerThreadPayloadBB()) {
        vASSERT(perThreadBB->Succs.size() == 1);
        mainBB = perThreadBB->Succs.front();
      }
    }

    INST_LIST_ITER insertBeforePos = mainBB->getFirstInsertPos();

    // Zero all address ARF
    G4_DstRegRegion *A0Dst =
        builder.createDst(builder.phyregpool.getAddrReg(), 0, 0, 1, Type_UD);
    G4_INST *zeroA0 =
        builder.createMov(g4::SIMD8, A0Dst, builder.createImm(0, Type_UD),
                          InstOpt_WriteEnable, false);
    (void)mainBB->insertBefore(insertBeforePos, zeroA0);

    // Zero acc ARF (at least two, some platform has more).
    G4_DstRegRegion *Acc0Dst =
        builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, Type_UD);
    G4_INST *zeroAcc0 = builder.createMov(builder.getNativeExecSize(), Acc0Dst,
                                          builder.createImm(0, Type_UD),
                                          InstOpt_WriteEnable, false);
    (void)mainBB->insertBefore(insertBeforePos, zeroAcc0);

    G4_DstRegRegion *Acc1Dst =
        builder.createDst(builder.phyregpool.getAcc1Reg(), 0, 0, 1, Type_UD);
    G4_INST *zeroAcc1 = builder.createMov(builder.getNativeExecSize(), Acc1Dst,
                                          builder.createImm(0, Type_UD),
                                          InstOpt_WriteEnable, false);
    (void)mainBB->insertBefore(insertBeforePos, zeroAcc1);

    // Zero flags
    int num32bitFlags = (int)(builder.getNumFlagRegisters() / 2);
    for (int i = 0; i < num32bitFlags; ++i) {
      G4_DstRegRegion *flagDst = builder.createDst(
          builder.phyregpool.getFlagAreg(i), 0, 0, 1, Type_UD);
      G4_INST *zeroFlag =
          builder.createMov(g4::SIMD1, flagDst, builder.createImm(0, Type_UD),
                            InstOpt_WriteEnable, false);
      (void)mainBB->insertBefore(insertBeforePos, zeroFlag);
    }
  }
}

void Optimizer::addSWSBInfo() {
  if (builder.hasDPAS() && builder.hasDPASFuseRSWA()) {
    // Currently the DPASFuseRSWA is tied to SWSB, so we make the
    // preparation work the first part of addSWSBInfo.
    prepareDPASFuseRSWA();
  }

  bool do_fcall_wa = builder.hasFusedEU() &&
                     builder.getuint32Option(vISA_fusedCallWA) == 1 &&
                     (kernel.fg.getHasStackCalls() || kernel.hasIndirectCall());

  if (do_fcall_wa) {
    // Need to be done before SWSB
    finishFusedCallWA_preSWSB();
  }

  if (!builder.hasSWSB()) {
    if (do_fcall_wa) {
      finishFusedCallWA();
    }
    return;
  }

  if (!builder.getOption(vISA_forceDebugSWSB)) {
    SWSB swsb(kernel);
    swsb.SWSBGenerator();
  } else {
    forceDebugSWSB(&kernel);
  }

  if (builder.getOptions()->getuInt32Option(vISA_SWSBTokenBarrier) != 0) {
    singleInstStallSWSB(
        &kernel, builder.getOptions()->getuInt32Option(vISA_SWSBTokenBarrier),
        0, true);
  }

  if (builder.getOptions()->getuInt32Option(vISA_SWSBInstStall) != 0) {
    singleInstStallSWSB(
        &kernel, builder.getOptions()->getuInt32Option(vISA_SWSBInstStall),
        builder.getOptions()->getuInt32Option(vISA_SWSBInstStallEnd), false);
  }

  if (do_fcall_wa) {
    // Need to be done when code is stable (no add, no delete).
    finishFusedCallWA();
  } else if (kernel.hasIndirectCall() && !builder.supportCallaRegSrc()) {
    adjustIndirectCallOffsetAfterSWSBSet();
  }
  return;
}

// Common pass for HW debug functions
void Optimizer::HWDebug() {
  if (builder.getOption(vISA_InsertHashMovs))
    insertHashMovs();
}

void Optimizer::insertHashMovs() {
  // As per request from IGC team, we want to conditionally insert
  // two mov instructions like following:
  //
  // send ... {EOT}
  // mov (16) null<1>:d        lo32 {NoMask}
  // mov (16) null<1>:d        hi32 {NoMask}
  //
  bool hashAtPrologue = kernel.getOption(vISA_HashMovsAtPrologue);
  for (G4_BB *bb : kernel.fg) {
    for (auto it = bb->begin(); it != bb->end(); ++it) {
      auto inst = (*it);
      if (inst->isEOT() || hashAtPrologue) {
        auto insertBefore = it;
        if (inst->isLabel())
          ++insertBefore;
        // We have to insert new instructions after EOT.
        // Lexically, EOT could even be in the middle
        // of the program.
        auto insertHashMovInsts = [&](uint64_t hashVal) {
          G4_INST *lo;
          G4_INST *hi;
          lo = kernel.fg.builder->createMov(
              g4::SIMD16, kernel.fg.builder->createNullDst(Type_UD),
              kernel.fg.builder->createImm((unsigned int)(hashVal & 0xffffffff),
                                           Type_UD),
              InstOpt_WriteEnable, false);

          hi = kernel.fg.builder->createMov(
              g4::SIMD16, kernel.fg.builder->createNullDst(Type_UD),
              kernel.fg.builder->createImm(
                  (unsigned int)((hashVal >> 32) & 0xffffffff), Type_UD),
              InstOpt_WriteEnable, false);
          // Option: -hashmovs hi lo
          //   To be consistent, 'mov hi' goes before 'mov lo'
          if (hashAtPrologue) {
            bb->insertBefore(insertBefore, hi);
            bb->insertBefore(insertBefore, lo);
          } else {
            bb->push_back(hi);
            bb->push_back(lo);
          }
        };

        // This func is called when vISA_HashVal is set by user;
        // but vISA_HashVal1 is still optional.
        uint64_t hashVal = builder.getOptions()->getuInt64Option(vISA_HashVal);
        insertHashMovInsts(hashVal);
        // vISA_HashVal1 is an extra hash value used to distinguish each entry
        // in the module. That works for IGC as IGC invokes vISA for each
        // kernel. However, VC invokes vISA for the whole module, so here we use
        // an unique id for the purpose. Note that if vISA_HashVal1 is given,
        // the value would still be used to emit the extra hash.
        if (builder.getOptions()->isOptionSetByUser(vISA_HashVal1)) {
          uint64_t hashVal1 =
            builder.getOptions()->getuInt64Option(vISA_HashVal1);
          insertHashMovInsts(hashVal1);
        } else if (kernel.getKernelType() == VISA_CM) {
          insertHashMovInsts(kernel.getFunctionId());
        }
        return;
      }
    }
  }
}

//
// Break a sample instruction
// send.smpl (16) dst src0 src1
// into
// (P1) send.smpl (16) dst src0 src1
// (~P1) send.smpl (16) dst src0 src1
// where P1 is 0x5555 (i.e., pixels with even x coordinates)
// Ideally this would only affect 3d textures, but at
// the moment it will affect 2d array textures as well.
//
// P1 is initialized per BB before the first sample inst; we could make it per
// shader but I'm worried about flag spill this works for SIMD8 and SIMD32
// shaders as well.
//
void Optimizer::cloneSampleInst() {
  bool cloneSample = builder.getOption(vISA_enableCloneSampleInst) &&
                     VISA_WA_CHECK(builder.getPWaTable(), Wa_14014414195);
  bool cloneEvaluateSample = builder.getOption(vISA_cloneEvaluateSampleInst);
  if (!cloneSample && !cloneEvaluateSample) {
    return;
  }

  bool isSIMD32 = kernel.getSimdSize() == 32;
  for (auto &&bb : kernel.fg) {
    auto tmpFlag = builder.createTempFlag(isSIMD32 ? 2 : 1);
    auto hasSample = false;
    for (auto I = bb->begin(), E = bb->end(); I != E;) {
      auto Next = std::next(I);
      auto inst = *I;
      if (inst->isSend() && inst->getMsgDesc()->getSFID() == SFID::SAMPLER &&
          inst->getMsgDescRaw() != nullptr &&
          inst->getExecSize() >= builder.getNativeExecSize()) {
        G4_InstSend *sendInst = inst->asSendInst();
        G4_Operand *src0 = sendInst->getSrc(0);

        unsigned int messageSizeInBytes =
            src0->getRightBound() - src0->getLeftBound() + 1;
        if (sendInst->isSplitSend()) {
          G4_Operand *src1 = sendInst->getSrc(1);
          messageSizeInBytes +=
              src1->getRightBound() - src1->getLeftBound() + 1;
        }
        if (sendInst->getMsgDescRaw()->isHeaderPresent()) {
          messageSizeInBytes -= kernel.getGRFSize();
        }
        unsigned int numParams = messageSizeInBytes / kernel.getGRFSize() *
                                 builder.getNativeExecSize() /
                                 inst->getExecSize();
        bool isEval = sendInst->getMsgDesc()->getDstLenRegs() == 0;
        uint32_t messageType =
            sendInst->getMsgDescRaw()->getSamplerMessageType();
        vISA_ASSERT(!inst->getPredicate(),
                    "do not handle predicated sampler inst for now");
        if (!isEval && cloneSample && messageType == 0 && numParams == 3) {
          if (!hasSample) {
            hasSample = true;
            auto flagInit = builder.createMov(
                g4::SIMD1,
                builder.createDst(tmpFlag->getRegVar(),
                                  isSIMD32 ? Type_UD : Type_UW),
                builder.createImm(isSIMD32 ? 0x55555555 : 0x5555,
                                  isSIMD32 ? Type_UD : Type_UW),
                InstOpt_WriteEnable, false);
            bb->insertBefore(I, flagInit);
          }
          auto newInst = inst->cloneInst();
          inst->setPredicate(
              builder.createPredicate(PredState_Plus, tmpFlag->getRegVar(), 0));
          newInst->setPredicate(builder.createPredicate(
              PredState_Minus, tmpFlag->getRegVar(), 0));
          auto newInstIt = bb->insertAfter(I, newInst);

          uint16_t rspLen =
              inst->asSendInst()->getMsgDescRaw()->ResponseLength();
          // If Pixel Null Mask feedback is requested sampler message
          // has header, all data channels enabled and an additional
          // GRF of writeback payload with Pixel Null Mask.
          // Possible message response lengths are:
          // - 5 GRFs for all simd8 messages and for simd16 messages
          //   with 16-bit return format
          // - 9 GRFs for simd16 message with 32-bit return format
          // It is enough to check send's response length to determine
          // if Pixel Null Mask feedback is enabled.
          vASSERT(inst->getExecSize() == g4::SIMD8 ||
                  inst->getExecSize() == g4::SIMD16);
          uint16_t pixelNullMaskRspLen =
              (inst->getExecSize() == g4::SIMD16 &&
               !sendInst->getMsgDescRaw()->is16BitReturn())
                  ? 9
                  : 5;

          if (sendInst->getMsgDescRaw()->isHeaderPresent() &&
              rspLen == pixelNullMaskRspLen) {
            // Pixel Null Mask is in the first word of the last GRF
            // of send's writeback message. This mask has bits set
            // to 0 for pixels in which a null page was source for
            // at least one texel. Otherwise bits are set to 1.

            // Create a copy of Pixel Null Mask from the first send
            // writeback message and AND it with the mask from the
            // second send.
            G4_Declare *maskCopy = builder.createTempVar(1, Type_UW, Any);
            G4_Declare *maskAlias = builder.createTempVar(1, Type_UW, Any);
            maskAlias->setAliasDeclare(
                inst->getDst()->getBase()->asRegVar()->getDeclare(),
                (inst->getDst()->getRegOff() + rspLen - 1) *
                    kernel.numEltPerGRF<Type_UB>());
            G4_SrcRegRegion *src = builder.createSrcRegRegion(
                maskAlias, builder.getRegionScalar());
            G4_DstRegRegion *dst =
                builder.createDst(maskCopy->getRegVar(), Type_UW);
            G4_INST *movInst = builder.createMov(g4::SIMD1, dst, src,
                                                 InstOpt_WriteEnable, false);
            bb->insertAfter(I, movInst);
            G4_SrcRegRegion *src0 = builder.createSrcRegRegion(*src);
            G4_SrcRegRegion *src1 =
                builder.createSrcRegRegion(maskCopy, builder.getRegionScalar());
            dst = builder.createDst(maskAlias->getRegVar(), Type_UW);
            G4_INST *andInst = builder.createBinOp(
                G4_and, g4::SIMD1, dst, src0, src1, InstOpt_WriteEnable, false);
            bb->insertAfter(newInstIt, andInst);
          }
        } else if (isEval && cloneEvaluateSample && messageType != 0x1F) {
          // 0x1F is the opcode for sampler cache flush
          uint32_t newExecSize =
              (messageType == VISA_3D_SAMPLE_L || messageType == VISA_3D_LD)
                  ? 8
                  : 1;
          uint32_t mask = (1 << newExecSize) - 1;
          auto evalTmpFlag = builder.createTempFlag(isSIMD32 ? 2 : 1);
          auto flagInit = builder.createMov(
              g4::SIMD1,
              builder.createDst(evalTmpFlag->getRegVar(),
                                isSIMD32 ? Type_UD : Type_UW),
              builder.createImm(mask, isSIMD32 ? Type_UD : Type_UW),
              InstOpt_WriteEnable, false);
          bb->insertBefore(I, flagInit);
          inst->setPredicate(builder.createPredicate(
              PredState_Plus, evalTmpFlag->getRegVar(), 0));
          unsigned numInsts = kernel.getSimdSize() / newExecSize;
          for (unsigned int i = 1; i < numInsts; i++) {
            auto newInst = inst->cloneInst();
            bb->insertAfter(I, newInst);
            evalTmpFlag = builder.createTempFlag(isSIMD32 ? 2 : 1);
            flagInit = builder.createMov(
                g4::SIMD1,
                builder.createDst(evalTmpFlag->getRegVar(),
                                  isSIMD32 ? Type_UD : Type_UW),
                builder.createImm(mask << (i * newExecSize),
                                  isSIMD32 ? Type_UD : Type_UW),
                InstOpt_WriteEnable, false);
            newInst->setPredicate(builder.createPredicate(
                PredState_Plus, evalTmpFlag->getRegVar(), 0));
            bb->insertAfter(I, flagInit);
          }
        }
      }
      I = Next;
    }
  }
}

void Optimizer::removeLifetimeOps() {
  // Call this function after RA only.

  // Remove all pseudo_kill and lifetime.end
  // instructions.
  // Also remove pseudo_use instructions.
  for (G4_BB *bb : kernel.fg) {
    bb->erase(std::remove_if(bb->begin(), bb->end(),
                             [](G4_INST *inst) {
                               return inst->isPseudoKill() ||
                                      inst->isLifeTimeEnd() ||
                                      inst->isPseudoUse();
                             }),
              bb->end());
  }
}

void Optimizer::runPass(PassIndex Index) {
  const PassInfo &PI = Passes[Index];

  // Do not execute.
  if ((PI.Option != vISA_EnableAlways && !builder.getOption(PI.Option)) ||
      EarlyExited)
    return;

  std::string Name = PI.Name;

#ifndef DLL_MODE
  if (StopBeforePass == Name) {
    EarlyExited = true;
    kernel.dumpToConsole();
    return;
  }
#endif // DLL_MODE

  setCurrentDebugPass(PI.Name);

  if (PI.Timer != TimerID::NUM_TIMERS)
    startTimer(PI.Timer);

  kernel.dumpToFile("before." + Name);

  // Execute pass.
  (this->*(PI.Pass))();

  if (PI.Timer != TimerID::NUM_TIMERS)
    stopTimer(PI.Timer);

  kernel.dumpToFile("after." + Name);
#ifndef DLL_MODE
  // Only check for stop-after in offline build as it's intended for vISA
  // debugging only. Note that stop-after does not work if the pass is not
  // executed.
  if (StopAfterPass == Name || EarlyExited) {
    EarlyExited = true;
    kernel.dumpToConsole();
  }
#endif // DLL_MODE

#ifdef _DEBUG
  bool skipVerify = Index == PI_regAlloc && (RAFail || EarlyExited);
  if (!skipVerify) {
    verifyG4Kernel(kernel, Index, true, G4Verifier::VC_ASSERT);
  }
#endif
  setCurrentDebugPass(nullptr);
}

void Optimizer::initOptimizations() {
#define OPT_INITIALIZE_PASS(Name, Option, Timer)                                   \
  Passes[PI_##Name] = PassInfo(&Optimizer::Name, "" #Name, Option, Timer)

  // To initialize a pass, the member function name is the first argument.
  // This member function must return void and take no argument.
  //
  // The second argument is the corresponding option to enable this pass.
  // If it always runs then use vISA_EnableAlways.
  //
  // The third argument is the intended timer for this pass. If no timing
  // is necessary, then TIMER_NUM_TIMERS can be used.
  //
  OPT_INITIALIZE_PASS(cleanMessageHeader, vISA_LocalCleanMessageHeader,
                  TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(forceNoMaskOnM0, vISA_forceNoMaskOnM0, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(sendFusion, vISA_EnableSendFusion, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(renameRegister, vISA_LocalRenameRegister, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(localDefHoisting, vISA_LocalDefHoist, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(localCopyPropagation, vISA_LocalCopyProp, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(localInstCombine, vISA_LocalInstCombine, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(removePartialMovs, vISA_RemovePartialMovs,
                  TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(cselPeepHoleOpt, vISA_enableCSEL, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(optimizeLogicOperation, vISA_EnableAlways,
                  TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(EmulateInt64Add, vISA_EnableAlways, TimerID::HW_CONFORMITY);
  OPT_INITIALIZE_PASS(HWConformityChk, vISA_EnableAlways, TimerID::HW_CONFORMITY);
  OPT_INITIALIZE_PASS(preRA_Schedule, vISA_preRA_Schedule,
                  TimerID::PRERA_SCHEDULING);
  OPT_INITIALIZE_PASS(preRA_HWWorkaround, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(preRegAlloc, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(regAlloc, vISA_EnableAlways, TimerID::TOTAL_RA);
  OPT_INITIALIZE_PASS(removeLifetimeOps, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(postRA_HWWorkaround, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(removeRedundMov, vISA_removeRedundMov, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(removeEmptyBlocks, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(insertFallThroughJump, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(reassignBlockIDs, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(evalAddrExp, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(FoldAddrImmediate, vISA_FoldAddrImmed, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(localSchedule, vISA_LocalScheduling, TimerID::SCHEDULING);
  OPT_INITIALIZE_PASS(HWWorkaround, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(fixEndIfWhileLabels, vISA_EnableAlways, TimerID::NUM_TIMERS);
  OPT_INITIALIZE_PASS(HWDebug, vISA_EnableAlways, TimerID::NUM_TIMERS);
  OPT_INITIALIZE_PASS(insertDummyMovForHWRSWA, vISA_InsertDummyMovForHWRSWA,
                  TimerID::NUM_TIMERS);
  OPT_INITIALIZE_PASS(insertDummyCompactInst, vISA_InsertDummyCompactInst,
                  TimerID::NUM_TIMERS);
  OPT_INITIALIZE_PASS(swapSrc1Src2OfMadForCompaction,
                      vISA_SwapSrc1Src2OfMadForCompaction,
                      TimerID::NUM_TIMERS);
  OPT_INITIALIZE_PASS(mergeScalarInst, vISA_MergeScalar, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(lowerMadSequence, vISA_EnableMACOpt, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(LVN, vISA_LVN, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(ifCvt, vISA_ifCvt, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(dumpPayload, vISA_dumpPayload, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(normalizeRegion, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(collectStats, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(createR0Copy, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(initializePayload, vISA_InitPayload, TimerID::NUM_TIMERS);
  OPT_INITIALIZE_PASS(cleanupBindless, vISA_enableCleanupBindless,
                  TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(cleanupA0Movs, vISA_enableCleanupA0Movs,
                  TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(countGRFUsage, vISA_PrintRegUsage, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(changeMoveType, vISA_ChangeMoveType, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(accSubBeforeRA, vISA_accSubBeforeRA, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(accSubPostSchedule, vISA_accSubstitution, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(s0SubAfterRA, vISA_EnableAlways, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(removePseudoMov, vISA_EnableAlways,
                  TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(dce, vISA_EnableDCE, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(reassociateConst, vISA_reassociate, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(split4GRFVars, vISA_split4GRFVar, TimerID::OPTIMIZER);
  OPT_INITIALIZE_PASS(loadThreadPayload, vISA_loadThreadPayload,
                  TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(addFFIDProlog, vISA_addFFIDProlog, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(addEmaskSetupProlog, vISA_addEmaskSetupProlog,
                  TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(insertFenceBeforeEOT, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(insertScratchReadBeforeEOT, vISA_clearScratchWritesBeforeEOT,
                  TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(mapOrphans, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(legalizeType, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(analyzeMove, vISA_analyzeMove, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(removeIntrinsics, vISA_EnableAlways, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(expandMulPostSchedule, vISA_expandMulPostSchedule,
                  TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(zeroSomeARF, vISA_zeroSomeARF, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(addSWSBInfo, vISA_addSWSBInfo, TimerID::SWSB);
  OPT_INITIALIZE_PASS(expandMadwPostSchedule, vISA_expandMadwPostSchedule,
                  TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(ACCSchedule, vISA_PreSchedForAcc, TimerID::PRERA_SCHEDULING);
  OPT_INITIALIZE_PASS(staticProfiling, vISA_staticProfiling, TimerID::MISC_OPTS);
  OPT_INITIALIZE_PASS(sinkBarrierWait, vISA_SinkBarrierWait,
                  TimerID::OPTIMIZER);

  // Verify all passes are initialized.
#ifdef _DEBUG
  for (unsigned i = 0; i < PI_NUM_PASSES; ++i) {
    vISA_ASSERT(Passes[i].Pass, "uninitialized pass");
  }
#endif
}

// simple heuristics to decide if it's profitable to do copy propagation for the
// move add more as necessary
bool Optimizer::isCopyPropProfitable(G4_INST *movInst) const {
  vISA_ASSERT(movInst->opcode() == G4_mov, "expected a move instruction");

  // if inst is a simd16 W/HF packing, we don't want to optimize it if
  // there are >=2 simd16 mad uses, since it will slow down the mad.
  // for gen9 additionally check for simd8 mad as it doesn't support strided
  // regions
  auto dst = movInst->getDst();
  auto src0 = movInst->getSrc(0);
  auto hasStrideSource = dst->getHorzStride() == 1 && src0->isSrcRegRegion() &&
                         !(src0->asSrcRegRegion()->getRegion()->isContiguous(
                               movInst->getExecSize()) ||
                           src0->asSrcRegRegion()->getRegion()->isScalar());

  hasStrideSource &=
      movInst->getExecSize() == g4::SIMD16 ||
      (!builder.hasAlign1Ternary() && movInst->getExecSize() == g4::SIMD8);

  auto hasNSIMD16or8MadUse = [](G4_INST *movInst, int N, bool checkSIMD8) {
    int numMadUses = 0;
    for (auto iter = movInst->use_begin(), iterEnd = movInst->use_end();
         iter != iterEnd; ++iter) {
      auto use = *iter;
      auto inst = use.first;
      if (inst->opcode() == G4_pseudo_mad &&
          (inst->getExecSize() == g4::SIMD16 ||
           (checkSIMD8 && inst->getExecSize() == g4::SIMD8))) {
        ++numMadUses;
        if (numMadUses == N) {
          return true;
        }
      }
    }
    return false;
  };

  if (hasStrideSource) {
    if (hasNSIMD16or8MadUse(movInst, 2, true)) {
      return false;
    }
  }

  // another condition where copy prop may be not profitable:
  // mov is from HF to F and dst is used in simd16 mad.
  // copy propagating away the move results in mix mode mad which is bad for
  // bank conflicts
  if (dst->getType() == Type_F && src0->getType() == Type_HF) {
    if (hasNSIMD16or8MadUse(movInst, 4, false)) {
      return false;
    }
  }
  return true;
}

void Optimizer::accSubPostSchedule() {
  if (!builder.doAccSub() || !builder.getOption(vISA_doAccSubAfterSchedule)) {
    return;
  }

  kernel.fg.resetLocalDataFlowData();
  kernel.fg.localDataFlowAnalysis();

  if (builder.getOption(vISA_localizationForAccSub)) {
    HWConformity hwConf(builder, kernel);
    for (auto bb : kernel.fg) {
      hwConf.localizeForAcc(bb);
    }

    kernel.fg.resetLocalDataFlowData();
    kernel.fg.localDataFlowAnalysis();
  }

  AccSubPass accSub(builder, kernel);
  accSub.run();
}

void Optimizer::s0SubAfterRA() {
  if (!builder.enableSendIndirect()) {
    return;
  }

  kernel.fg.resetLocalDataFlowData();
  kernel.fg.localDataFlowAnalysis();

  SRSubPassAfterRA s0Sub(builder, kernel);
  s0Sub.run();
}

void Optimizer::accSubBeforeRA() {
  if (!builder.doAccSub() || !builder.getOption(vISA_doAccSubAfterSchedule)) {
    return;
  }

  kernel.fg.resetLocalDataFlowData();
  kernel.fg.localDataFlowAnalysis();

  if (builder.getOption(vISA_localizationForAccSub)) {
    HWConformity hwConf(builder, kernel);
    for (auto bb : kernel.fg) {
      hwConf.localizeForAcc(bb);
    }

    kernel.fg.resetLocalDataFlowData();
    kernel.fg.localDataFlowAnalysis();
  }

  AccSubPass accSub(builder, kernel);
  accSub.run();
}

bool Optimizer::R0CopyNeeded() {
  if (!builder.canReadR0()) {
    // If r0 cannot be read then r0 has to be copied
    // and cannot be said to be preserved in r0. In
    // other words, these 2 are mutually exclusive
    // options.
    vISA_ASSERT(!kernel.getOption(vISA_PreserveR0InR0),
                "opposing options for r0 detected");
    return true;
  }

  if (kernel.getOption(vISA_PreserveR0InR0)) {
    return false;
  }

  if (builder.getIsKernel() && kernel.fg.getHasStackCalls()) {
    // As per VISA ABI, last register in GRF file should
    // contain copy of r0.
    return true;
  }

  return false;
}

int Optimizer::optimization() {
#ifndef DLL_MODE
  if (StopAfterPass == "CFGConstruction") {
    EarlyExited = true;
    kernel.dumpToConsole();
  }
#endif // DLL_MODE

  // remove redundant message headers.
  runPass(PI_cleanMessageHeader);

  // Set NoMask inst's mask offset to 0 if possible
  runPass(PI_forceNoMaskOnM0);


  runPass(PI_sendFusion);

  // rename registers.
  runPass(PI_renameRegister);

  runPass(PI_localDefHoisting);

  runPass(PI_removePartialMovs);

  runPass(PI_cleanupA0Movs);

  // remove redundant movs and fold some other patterns
  runPass(PI_localCopyPropagation);

  // fold some binary operations
  runPass(PI_localInstCombine);

  runPass(PI_mergeScalarInst);

  runPass(PI_cselPeepHoleOpt);

  runPass(PI_reassociateConst);

  runPass(PI_lowerMadSequence);

  // optimize logic operantions
  runPass(PI_optimizeLogicOperation);

  // Dead code elimination
  runPass(PI_dce);

  // Emulate 64 Int add if needed
  runPass(PI_EmulateInt64Add);

  // HW conformity check
  runPass(PI_HWConformityChk);

  // Local Value Numbering
  runPass(PI_LVN);

  // this must be run after copy prop cleans up the moves
  runPass(PI_cleanupBindless);

  runPass(PI_split4GRFVars);

  runPass(PI_insertFenceBeforeEOT);


  // PreRA scheduling
  runPass(PI_preRA_Schedule);

  // HW workaround before RA
  runPass(PI_preRA_HWWorkaround);

  if (builder.enableACCBeforRA() && builder.enablePreSchedACC()) {
    runPass(PI_ACCSchedule);
  }

  if (builder.enableACCBeforRA() && !builder.enablePreSchedACC()) {
    runPass(PI_accSubBeforeRA);
  }

  runPass(PI_preRegAlloc);

  // perform register allocation
  runPass(PI_regAlloc);
  if (RAFail) {
    return VISA_SPILL;
  }



  runPass(PI_removeLifetimeOps);

  // HW workaround after RA
  runPass(PI_postRA_HWWorkaround);

  //
  // if a fall-through BB does not immediately follow its predecessor
  // in the code layout, then insert a jump-to-fall-through in the predecessor
  //
  runPass(PI_insertFallThroughJump);

  // Run if-conversion to convert short if-blocks.
  runPass(PI_ifCvt);

  //
  // re-assign block ID so that we can use id to determine the ordering of
  // two blocks in the code layout
  //
  runPass(PI_reassignBlockIDs);

  runPass(PI_FoldAddrImmediate);

  // FIXME houjenko: Disable local scheduling due to issues when
  // using extra regiser that may corrupt unknown liveout
  if (!builder.getIsPayload()) {
    runPass(PI_localSchedule);
  }

  if (!builder.enableACCBeforRA() && !builder.enablePreSchedACC()) {
    runPass(PI_expandMulPostSchedule);

    runPass(PI_expandMadwPostSchedule);

    runPass(PI_accSubPostSchedule);
  }

  runPass(PI_legalizeType);

  runPass(PI_changeMoveType);
  runPass(PI_s0SubAfterRA);

  // No pass after this should expect def-use to be preserved as this pass
  // removes raw movs with identical src/dst physical GRFs.
  runPass(PI_removeRedundMov);

  // remove any placeholders blocks inserted to aid regalloc
  // run this pass after reRA pass otherwise CFG can become
  // invalid (funcInfo, calleeInfo may point to bad initBB).
  runPass(PI_removeEmptyBlocks);

  runPass(PI_insertScratchReadBeforeEOT);

  runPass(PI_sinkBarrierWait);
  // HW workaround
  runPass(PI_HWWorkaround);

  runPass(PI_normalizeRegion);

  runPass(PI_countGRFUsage);

  runPass(PI_dumpPayload);

  // this must be the last step of the optimization so as to not violate
  // the CFG assumption
  runPass(PI_fixEndIfWhileLabels);

  runPass(PI_HWDebug);

  runPass(PI_insertDummyMovForHWRSWA);

  runPass(PI_collectStats);

  // Create a copy of R0 at the top of kernel.
  // This must be done after all other optimizer
  // passes except for loadThreadPlayoad
  runPass(PI_createR0Copy);

  runPass(PI_initializePayload);

  runPass(PI_loadThreadPayload);

  runPass(PI_addFFIDProlog);

  runPass(PI_addEmaskSetupProlog);

  // Insert a dummy compact instruction if requested for SKL+
  runPass(PI_insertDummyCompactInst);

  runPass(PI_swapSrc1Src2OfMadForCompaction);

  runPass(PI_mapOrphans);

  runPass(PI_analyzeMove);

  runPass(PI_removeIntrinsics);

  runPass(PI_zeroSomeARF);

  //-----------------------------------------------------------------------------------------------------------------
  //------NOTE!!!! No instruction change(add/remove, or operand associated
  // change) is allowed after SWSB-------------
  //-----------------------------------------------------------------------------------------------------------------
  runPass(PI_addSWSBInfo);

  runPass(PI_removePseudoMov);

  runPass(PI_staticProfiling);

  if (EarlyExited) {
    return VISA_EARLY_EXIT;
  }
  return VISA_SUCCESS;
}

//  When constructing CFG we have the assumption that a label must be the first
//  instruction in a bb.  During structure analysis, however, we may end up with
//  a bb that starts with multiple endifs if the bb is the target of multiple
//  gotos that have been converted to an if. Instead of creating a BB for each
//  of the endif, we associate each endif with a label and emit them only at the
//  very end.
//
//  For break and continue, UIP must be the lable directly attached to the while
//  op. If not, create such a label
//
//  DO
//    IF
//      P =
//      CONT L1
//    ENDIF L1
//    IF
//      BREAK L2
//    ENDIF L1
//  L1
//  (P) WHILE
//  L2
//
//  will be transfered into
//
//  DO
//    IF
//      P =
//      Spill <- P
//      CONT L3         // UIP becomes L3
//    ENDIF L1
//    IF
//      BREAK L3        // UIP becomes L3
//    ENDIF L1
//  L1                  // existing label
//  P <- fill
//  L3                  // new label
//  (P) WHILE
//  L2
//
void Optimizer::fixEndIfWhileLabels() {
  for (BB_LIST_CITER iter = fg.cbegin(), bend = fg.cend(); iter != bend;
       ++iter) {
    G4_BB *bb = *iter;
    INST_LIST_ITER iter2 = bb->begin();
    INST_LIST_ITER iend = bb->end();
    while (iter2 != iend) {
      INST_LIST_ITER currIter = iter2;
      ++iter2;

      G4_INST *inst = *currIter;
      G4_Label *endifLabel = fg.getLabelForEndif(inst);
      if (endifLabel) {
        G4_INST *labelInst = fg.createNewLabelInst(endifLabel);
        bb->insertBefore(currIter, labelInst);
      }
    }
  }

  // Patch labels if necessary.
  for (G4_BB *bb : fg) {
    if (bb->empty())
      continue;

    G4_INST *inst = bb->back();
    G4_opcode opc = inst->opcode();
    if (opc != G4_cont && opc != G4_break)
      continue;

    // The matching while BB.
    G4_BB *whileBB = nullptr;
    if (opc == G4_cont) {
      // The whileBB is the first successor bb, if this is continue.
      whileBB = bb->Succs.front();
    } else {
      // For break, the whileBB should be the physical predecessor of
      // break's first successor bb.
      for (G4_BB *succBB : bb->Succs) {
        if (succBB->getPhysicalPred() &&
            (!succBB->getPhysicalPred()->empty()) &&
            (succBB->getPhysicalPred()->back()->opcode() == G4_while)) {
          whileBB = succBB->getPhysicalPred();
          break;
        }
      }
    }

    if (whileBB == nullptr || whileBB->empty() ||
        whileBB->back()->opcode() != G4_while) {
      vISA_ASSERT(false, "can not find while BB");
      continue;
    }

    // If while instruction is following the label, then no need
    // to insert a new uip label, just use the existing one.
    G4_InstCF *instCF = inst->asCFInst();
    auto whileIter = std::prev(whileBB->end());
    G4_INST *prevInst = *std::prev(whileIter);
    if (prevInst->isLabel()) {
      instCF->setUip(prevInst->getLabel());
    } else {
      std::string NewUipName = instCF->getUipLabelStr();
      NewUipName += "_UIP";
      G4_Label *label = builder.createLabel(NewUipName, LABEL_BLOCK);
      instCF->setUip(label);

      G4_INST *newInst = fg.createNewLabelInst(label);

      whileBB->insertBefore(whileIter, newInst);
    }
  }
}

// Fold address register into address register offset, such that we can same one
// instruction that computes: add a0.0 a0.0 immed; mul dst r[a0.0, 0] src2
//-->
// mul dst r[a0.0, immed] src2

// The condition is that immed is in range [-512..511] and it is dividable
// by 32. This is a local OPT. For simplicity, only execsize 1 is considered.
// Since physical registers are alreasy assigned, we use the info directly here
// without check.

void Optimizer::reverseOffsetProp(AddrSubReg_Node addrRegInfo[8], int subReg,
                                  unsigned int srcNum, INST_LIST_ITER lastIter,
                                  INST_LIST_ITER iend) {
  if (addrRegInfo[subReg].usedImmed && addrRegInfo[subReg].canUseImmed) {
    INST_LIST_ITER iter;
    G4_INST *inst;
    G4_Operand *inst_src;
    G4_DstRegRegion *inst_dst;
    for (iter = addrRegInfo[subReg].iter; iter != lastIter; ++iter) {
      if (iter == lastIter)
        break;
      inst = *iter;
      if (inst->isDead())
        continue;
      inst_dst = inst->getDst();
      if (inst_dst && inst_dst->getRegAccess() != Direct) {
        int subReg1 = getDstSubReg(inst_dst);

        short currOff = inst_dst->getAddrImm();
        if (subReg1 == subReg) {
          // create a new dst
          G4_DstRegRegion tmpRgn(*inst_dst);
          G4_DstRegRegion *newDst = &tmpRgn;
          newDst->setImmAddrOff(
              short(currOff - addrRegInfo[subReg].immAddrOff));
          inst->setDest(builder.createDstRegRegion(*newDst));
        }
      }
      for (int i = 0; i < inst->getNumSrc(); i++) {
        inst_src = inst->getSrc(i);
        if (inst_src && inst_src->isSrcRegRegion() &&
            inst_src->asSrcRegRegion()->getRegAccess() != Direct) {
          int subReg1 = getSrcSubReg(inst_src);

          short currOff = inst_src->asSrcRegRegion()->getAddrImm();
          if (subReg1 == subReg) {
            G4_SrcRegRegion tmpRgn(*inst_src->asSrcRegRegion());
            G4_SrcRegRegion *newSrc = &tmpRgn;
            newSrc->setImmAddrOff(
                short(currOff - addrRegInfo[subReg].immAddrOff));
            inst->setSrc(builder.createSrcRegRegion(*newSrc), i);
          }
        }
      }
    }
    // Immed has been propagated to srcs before this src in *ii, also reverse
    // this
    if (srcNum > 0) {
      inst = *lastIter;
      for (unsigned i = 0; i < srcNum; i++) {
        inst_src = inst->getSrc(i);
        if (inst_src && inst_src->isSrcRegRegion() &&
            inst_src->asSrcRegRegion()->getRegAccess() != Direct) {
          int subReg1 = getSrcSubReg(inst_src);

          short currOff = inst_src->asSrcRegRegion()->getAddrImm();
          if (subReg1 == subReg) {
            G4_SrcRegRegion tmpRgn(*inst_src->asSrcRegRegion());
            G4_SrcRegRegion *newSrc = &tmpRgn;
            newSrc->setImmAddrOff(
                short(currOff - addrRegInfo[subReg].immAddrOff));
            inst->setSrc(builder.createSrcRegRegion(*newSrc), i);
          }
        }
      }
    }
  }

  addrRegInfo[subReg].immAddrOff = 0;
  addrRegInfo[subReg].iter = iend;
  addrRegInfo[subReg].canRemoveInst = false;
  addrRegInfo[subReg].canUseImmed = false;
  addrRegInfo[subReg].usedImmed = false;
}

void Optimizer::removePseudoMov() {
  if (!builder.enableSendIndirect()) {
    return;
  }

  for (G4_BB *bb : fg) {
    INST_LIST_ITER ii(bb->begin()), iend(bb->end());
    while (ii != iend) {
      G4_INST *inst = (*ii);

      if (inst->isPseudoAddrMovIntrinsic()) {
        uint64_t value = 0;

        for (int i = 0; i < inst->getNumSrc(); i++) {
          G4_Operand *src = inst->getSrc(i);

          if (!src || src->isNullReg()) {
            continue;
          }

          vASSERT(src->isAddrExp());
          G4_RegVar *regVar = src->asAddrExp()->getRegVar();
          vASSERT(regVar->getPhyReg()->isGreg());

          unsigned int regNum =
              (static_cast<G4_Greg *>(regVar->getPhyReg()))->getRegNum();
          regNum += src->asAddrExp()->getOffset() / kernel.getGRFSize();
          value |= (uint64_t)regNum << 8 * i;
        }
        G4_Imm *src = builder.createImm(value, Type_UQ);
        G4_INST *movInst = builder.createMov(g4::SIMD1, inst->getDst(), src,
                                             InstOpt_WriteEnable, false);
        movInst->setToken(inst->getToken());
        movInst->setTokenType(inst->getTokenType());
        movInst->setDistance(inst->getDistance());
        movInst->setDistanceTypeXe(inst->getDistanceTypeXe());
        bb->insertBefore(ii, movInst);
        INST_LIST_ITER tmp = ii;
        ii++;
        bb->erase(tmp);
        continue;
      }
      ii++;
    }
  }
}
void Optimizer::FoldAddrImmediate() {
  AddrSubReg_Node *addrRegInfo =
      new AddrSubReg_Node[builder.getNumAddrRegisters()];
  int dst_subReg = 0, src0_subReg = 0;
  G4_DstRegRegion *dst;
  G4_Operand *src0, *src1;
  unsigned num_srcs;

  for (G4_BB *bb : fg) {
    INST_LIST_ITER ii, iend(bb->end());
    // reset address offset info
    for (unsigned i = 0; i < builder.getNumAddrRegisters(); i++) {
      addrRegInfo[i].subReg = 0;
      addrRegInfo[i].immAddrOff = 0;
      addrRegInfo[i].iter = iend;
      addrRegInfo[i].canRemoveInst = false;
      addrRegInfo[i].canUseImmed = false;
      addrRegInfo[i].usedImmed = false;
    }
    for (ii = bb->begin(); ii != iend; ii++) {
      G4_INST *inst = *ii;
      if (inst->isDead()) {
        continue;
      }
      num_srcs = inst->getNumSrc();
      dst = inst->getDst();
      if (dst) {
        dst_subReg = getDstSubReg(dst);
      }
      src0 = inst->getSrc(0);
      if (src0 && src0->isSrcRegRegion()) {
        src0_subReg = getSrcSubReg(src0);
      }
      src1 = inst->getSrc(1);

      if (dst && dst->isDirectA0() && src0 && src0->isSrcRegRegion() &&
          src0->asSrcRegRegion()->isDirectA0() && !src1) {
        continue;
      }

      if (inst->opcode() == G4_add && inst->getExecSize() == g4::SIMD1 &&
          !inst->getPredicate() && (src1->isImm() && !src1->isRelocImm()) &&
          dst && dst->isDirectA0() && src0 && src0->isSrcRegRegion() &&
          src0->asSrcRegRegion()->isDirectA0() && dst_subReg == src0_subReg) {
        // since there is use of a0.x here, we can not remove the former def of
        // a0.x reverse immed offset propagation
        reverseOffsetProp(addrRegInfo, dst_subReg, 0, ii, iend);

        int64_t offset = src1->asImm()->getImm();
        if (offset >= -512 && offset <= 511 && offset % 0x20 == 0) {
          // this kills the previous def on a0.x
          if (addrRegInfo[dst_subReg].canRemoveInst &&
              addrRegInfo[dst_subReg].iter != iend) {
            // mark dead
            (*(addrRegInfo[dst_subReg].iter))->markDead();
          }
          addrRegInfo[dst_subReg].subReg = dst_subReg;
          addrRegInfo[dst_subReg].immAddrOff = (short)offset;
          addrRegInfo[dst_subReg].iter = ii;
          addrRegInfo[dst_subReg].canRemoveInst = true;
          addrRegInfo[dst_subReg].canUseImmed = true;
          addrRegInfo[dst_subReg].usedImmed = false;
        }
      } else {
        G4_Operand *src;
        // if there is any direct use of addr reg, the ADD inst can not be
        // removed
        for (unsigned i = 0; i < num_srcs; i++) {
          src = inst->getSrc(i);
          if (src && src->isSrcRegRegion() &&
              src->asSrcRegRegion()->isDirectA0()) {
            // TODO: show if an inst is generated for spill code
            // if there is no regVar for this srcRegion, the physical register
            // is hard-wired in input or generated by spillCode. in this case,
            // the subregister info is in the subRegOff of G4_SrcRegRegion this
            // also applies to dst register
            int subReg = getSrcSubReg(src);

            // it is possible that several elements are used
            int width, hstride, vstride, outerloop = 1;
            width = src->asSrcRegRegion()->getRegion()->width;
            hstride = src->asSrcRegRegion()->getRegion()->horzStride;
            vstride = src->asSrcRegRegion()->getRegion()->vertStride;
            if (vstride != 0) {
              outerloop = inst->getExecSize() / vstride;
            }

            for (int k = 0; k < outerloop; k++) {
              for (int j = 0; j < width; j++) {
                int currSubreg = subReg + k * vstride + j * hstride;
                // there may be inst whose src or dst addr immediate offset are
                // already changed reverse the change
                reverseOffsetProp(addrRegInfo, currSubreg, i, ii, iend);
              }
            }
          }
        }
        // use of address register in index region
        for (unsigned i = 0; i < num_srcs; i++) {
          src = inst->getSrc(i);
          if (src && src->isSrcRegRegion() &&
              src->asSrcRegRegion()->getRegAccess() != Direct) {
            int subReg = getSrcSubReg(src);

            // if VxH is used and more than one sub registers are used in
            // addressing do not fold the immediate even though they have the
            // same immediate value
            unsigned short vertStride =
                src->asSrcRegRegion()->getRegion()->vertStride;
            if (vertStride == UNDEFINED_SHORT ||
                (vertStride > 0 &&
                 (unsigned short)inst->getExecSize() / vertStride > 1)) {
              int numSubReg = 0;
              if (vertStride == UNDEFINED_SHORT) {
                numSubReg = inst->getExecSize() /
                            src->asSrcRegRegion()->getRegion()->width;
              } else {
                numSubReg = 1; // inst->getExecSize()/vertStride;
              }
              for (int j = subReg; j < subReg + numSubReg; j++) {
                reverseOffsetProp(addrRegInfo, j, i, ii, iend);
              }
            } else {
              // we check the existing address reg imm offset.
              short currOff = src->asSrcRegRegion()->getAddrImm();
              if (addrRegInfo[subReg].canUseImmed) {
                if (currOff % 0x20 == 0 &&
                    (currOff + addrRegInfo[subReg].immAddrOff) <= 511 &&
                    (currOff + addrRegInfo[subReg].immAddrOff) >= -512) {
                  G4_SrcRegRegion tmpRgn(*src->asSrcRegRegion());
                  G4_SrcRegRegion *newSrc = &tmpRgn;
                  newSrc->setImmAddrOff(
                      short(currOff + addrRegInfo[subReg].immAddrOff));
                  inst->setSrc(builder.createSrcRegRegion(*newSrc), i);

                  addrRegInfo[subReg].usedImmed = true;
                } else {
                  // if the offset can not be folded into all uses of a0.0,
                  // reverse the former folding
                  reverseOffsetProp(addrRegInfo, subReg, i, ii, iend);
                }
              }
            }
          }
        }
        if (dst) {
          // make sure the addr reg is not redefined
          // direct access to a0.x
          if (dst->isDirectA0()) {
            int width, hstride;
            width = inst->getExecSize();
            hstride = dst->getHorzStride();

            for (int j = 0; j < width; j++) {
              int currSubreg = dst_subReg + j * hstride;
              // this kills the previous def on a0.x
              if (addrRegInfo[currSubreg].iter != iend &&
                  addrRegInfo[currSubreg].canRemoveInst) {
                // mark dead
                (*(addrRegInfo[currSubreg].iter))->markDead();
              }
              addrRegInfo[currSubreg].immAddrOff = 0;
              addrRegInfo[currSubreg].iter = iend;
              addrRegInfo[currSubreg].canRemoveInst = false;
              addrRegInfo[currSubreg].canUseImmed = false;
              addrRegInfo[currSubreg].usedImmed = false;
            }
          }
          // check if dst is indirectly addressed
          else if (dst->getRegAccess() != Direct) {
            short currOff = dst->getAddrImm();
            if (addrRegInfo[dst_subReg].canUseImmed) {
              if (currOff % 0x20 == 0 &&
                  (currOff + addrRegInfo[dst_subReg].immAddrOff) <= 511 &&
                  (currOff + addrRegInfo[dst_subReg].immAddrOff) >= -512) {
                // create a new dst
                G4_DstRegRegion tmpRgn(*dst);
                G4_DstRegRegion *newDst = &tmpRgn;
                newDst->setImmAddrOff(
                    short(currOff + addrRegInfo[dst_subReg].immAddrOff));
                inst->setDest(builder.createDstRegRegion(*newDst));
                addrRegInfo[dst_subReg].usedImmed = true;
              } else {
                // if the offset can not be folded into all uses of a0.0,
                // reverse the former folding
                reverseOffsetProp(addrRegInfo, dst_subReg, 0, ii, iend);
              }
            }
          }
        }
      }
    }

    // if a def lives out of this BB, we can not delete the defining inst
    for (unsigned i = 0; i < builder.getNumAddrRegisters(); i++) {
      // reverse immed offset propagation
      reverseOffsetProp(addrRegInfo, i, 0, iend, iend);
    }
    // remove the ADD instructions that marked as dead
    for (ii = bb->begin(); ii != bb->end();) {
      G4_INST *inst = *ii;
      INST_LIST_ITER curr = ii++;
      if (inst->isDead()) {
        bb->erase(curr);
      }
    }
  }

  delete[] addrRegInfo;
}
G4_SrcModifier Optimizer::mergeModifier(G4_Operand *src, G4_Operand *use) {
  if ((src == NULL || !src->isSrcRegRegion()) && use && use->isSrcRegRegion()) {
    return use->asSrcRegRegion()->getModifier();
  } else if ((use == NULL || !use->isSrcRegRegion()) && src &&
             src->isSrcRegRegion()) {
    return src->asSrcRegRegion()->getModifier();
  } else if (src && src->isSrcRegRegion() && use && use->isSrcRegRegion()) {
    G4_SrcModifier mod1 = src->asSrcRegRegion()->getModifier(),
                   mod2 = use->asSrcRegRegion()->getModifier();
    if (mod2 == Mod_Abs || mod2 == Mod_Minus_Abs) {
      return mod2;
    } else if (mod2 == Mod_src_undef) {
      return mod1;
    } else {
      // mod2 == Minus
      if (mod1 == Mod_Minus) {
        return Mod_src_undef;
      } else if (mod1 == Mod_Abs) {
        return Mod_Minus_Abs;
      } else if (mod1 == Mod_Minus_Abs) {
        return Mod_Abs;
      } else {
        return mod2;
      }
    }
  } else {
    return Mod_src_undef;
  }
}

// Prevent sinking in presence of lifetime.end for any src op.
// For eg,
// add V33, V32, 0x1
// ...
// lifetime.end V32 <-- This prevents sinking of add to mov
// ...
// pseudo_kill V34 <-- This prevents hoisting of V34 to add dst
// mov V34, V33
//
static bool checkLifetime(G4_INST *defInst, G4_INST *inst) {
  // Check whether current instruction ends any src opnd of op
  if (!inst->isLifeTimeEnd())
    return true;

  G4_RegVar *Var = GetTopDclFromRegRegion(inst->getSrc(0))->getRegVar();
  // Check whether lifetime op corresponds to any operand of current inst.
  if (defInst->getPredicate()) {
    G4_RegVar *opndVar =
        defInst->getPredicate()->asPredicate()->getBase()->asRegVar();
    if (opndVar == Var)
      return false;
  }
  if (defInst->getCondMod()) {
    G4_RegVar *opndVar =
        defInst->getCondMod()->asCondMod()->getBase()->asRegVar();
    if (opndVar == Var)
      return false;
  }
  if (defInst->getDst() && !defInst->getDst()->isNullReg()) {
    G4_RegVar *opndVar = GetTopDclFromRegRegion(defInst->getDst())->getRegVar();
    if (opndVar == Var)
      return false;
  }
  for (unsigned int srcOpnd = 0, numSrc = defInst->getNumSrc();
       srcOpnd < numSrc; srcOpnd++) {
    G4_Operand *src = defInst->getSrc(srcOpnd);
    if (src && src->isSrcRegRegion()) {
      G4_RegVar *opndVar = GetTopDclFromRegRegion(src)->getRegVar();
      if (opndVar == Var)
        return false;
    }
  }

  return true;
}

//
// Sink definition towards its use.
//
// For example, without sinking the def instruction once, use cannot
// be hoisted, since there is a data dependency between the middle
// instruction and the last move.
//
// def: shr (1) V39(0,0)<1>:ud V38(0,0)<0;1,0>:d 0x4:w {Align1, Q1}
//      mov (8) V68(0,0)<1>:ud r0.0<8;8,1>:ud {Align1, NoMask}
// use: mov (1) V68(0,2)<1>:ud V39(0,0)<0;1,0>:ud {Align1, NoMask}
//
// after sinking, it becomes
//
//      mov (8) V68(0,0)<1>:ud r0.0<8;8,1>:ud {Align1, NoMask}
// def: shr (1) V39(0,0)<1>:ud V38(0,0)<0;1,0>:d 0x4:w {Align1, Q1}
// use: mov (1) V68(0,2)<1>:ud V39(0,0)<0;1,0>:ud {Align1, NoMask}
//
// which makes local def hoisting possible.
//
// The third argument 'other' points to the first instruction (upwards) that
// has data-dependency with the use instruction.
//
static bool canSink(G4_BB *bb, INST_LIST_RITER revIter, INST_LIST_RITER other) {
  // The use instruction.
  G4_INST *inst = *revIter;

  // Currently we do not handle multiple definition for this optimization.
  if (inst->def_size() != 1)
    return false;

  // Find its def instruction.
  G4_INST *defInst = inst->def_back().first;

  vISA_ASSERT(*other != defInst, "iterator points to def already");

  // Walk up to check if sinking is safe.
  INST_LIST_RITER it = other;

  while (*it != defInst) {
    if ((*it)->isWAWdep(defInst) || (*it)->isRAWdep(defInst) ||
        (*it)->isWARdep(defInst))
      return false;

    if (!checkLifetime(defInst, *it))
      return false;

    // move towards to defInst.
    ++it;
  }

  // At this point, there is no data dependency and sinking is safe.
  //
  // We do sinking right here.
  //
  vISA_ASSERT(*it == defInst, "iterator out of sync");

  // Both 'other' and 'it' are reverse iterators, and sinking is through
  // forward iterators. The fisrt base should not be decremented by 1,
  // otherwise, the instruction will be inserted before not after.
  bb->insertBefore(other.base(), defInst);
  bb->erase(--it.base());

  return true;
}

static bool canHoist(FlowGraph &fg, G4_BB *bb, INST_LIST_RITER revIter) {
  G4_INST *inst = *revIter;

  if (inst->isMixedMode() && fg.builder->getOption(vISA_DisableleHFOpt))
    return false;
  // Cannot hoist if this is not a move, or it is a global operand.
  if (inst->opcode() != G4_mov ||
      fg.globalOpndHT.isOpndGlobal(inst->getSrc(0)) ||
      !inst->canHoist(!bb->isAllLaneActive(), fg.builder->getOptions())) {
    return false;
  }

  if (auto Dst = inst->getDst()) {
    G4_Declare *Dcl = Dst->getTopDcl();
    // Do not do def-hoisting for setting flags which is likely to increase flag
    // register pressure.
    if (Dcl && Dcl->getRegFile() == G4_RegFileKind::G4_FLAG) {
      return false;
    }

    // Do not do def-hoisting for s0 registers
    if (Dcl && Dcl->getRegFile() == G4_RegFileKind::G4_SCALAR) {
      return false;
    }

    if (Dcl && Dcl->getRegFile() == G4_RegFileKind::G4_ADDRESS &&
        Dcl->getRegVar() && Dcl->getRegVar()->getPhyReg()) {
      // Dont def-hoist if dst is hardwired to address register.
      // Doing so extends live-range of assigned register a0.
      // Given that the machine has single addr register, a0,
      // it may even cause address RA to fail due to uncolorable
      // graph.
      return false;
    }

    if (!fg.builder->hasByteALU() && Dst->getTypeSize() == 1) {
      return false;
    }
  }

  // Now check each definition of src(0)
  for (auto I = inst->def_begin(), E = inst->def_end(); I != E; ++I) {
    vISA_ASSERT(I->second == Opnd_src0, "invalid use-def chain");
    if (!inst->canHoistTo(I->first, !bb->isAllLaneActive()))
      return false;

    auto defInst = I->first;
    if (fg.globalOpndHT.isOpndGlobal(defInst->getDst())) {
      return false;
    }

    auto defSrc0 = defInst->getSrc(0);
    if (inst->getDst()->getType() == Type_BF &&
        (defSrc0->getType() != Type_F ||
         (defInst->isMov() &&
          defSrc0->isSrcRegRegion() &&
          defSrc0->asSrcRegRegion()->hasModifier()))) {
      // we currently don't handle conversion to BF from other type than float
      // As F->BF does not support srcMod, cannot hoist if definst has mod.
      return false;
    }
    // don't hoist if defInst could become a movi (localCopyProp is later pass)
    if (fg.builder->canPromoteToMovi(defInst)) {
      return false;
    }

    // Further check data-dependency, that is, no other instruction
    // should have WAR or WAW dependency with this inst.
    //
    //   defInst
    //
    //   other inst
    //
    //   inst <-- revIter
    //
    INST_LIST_RITER other = revIter;
    ++other;

    // Measure the distance in between
    unsigned distance = 0;

    // Walkup until hits its defining instruction.
    while (*other != I->first) {
      // FIXME: remove duplicate computations for multiple definitions.
      if (inst->isWAWdep(*other) || inst->isWARdep(*other)) {
        break;
      }
      ++other;
      ++distance;
    }

// Check the distance first, if this is too far then the following
// sinking optimization is very expensive.
#define MAX_DEF_HOIST_DIST 160
    if (distance > MAX_DEF_HOIST_DIST)
      return false;

    // There is a data dependency.
    if (*other != I->first) {
      // check if sinking is possible.
      if (!canSink(bb, revIter, other))
        return false;
    }
  }
  return true;
}

static G4_DstRegRegion *buildNewDstOperand(FlowGraph &fg, G4_INST *inst,
                                           G4_INST *defInst) {
  G4_Operand *src = inst->getSrc(0);
  G4_DstRegRegion *dst = inst->getDst();

  G4_Type srcType = src->getType();
  G4_Type dstType = dst->getType();
  G4_DstRegRegion *dstRegion = dst;
  bool indirectDst = (dstRegion->getRegAccess() != Direct);
  unsigned char srcElSize = (unsigned char)TypeSize(srcType);

  G4_DstRegRegion *defDstRegion = defInst->getDst();
  G4_DstRegRegion *newDstOpnd = dst;

  unsigned char defDstElSize = (unsigned char)defDstRegion->getTypeSize();
  G4_CmpRelation rel = src->compareOperand(defDstRegion, *fg.builder);
  G4_Type defDstType = defDstRegion->getType();

  unsigned char dstElSize = (unsigned char)TypeSize(dstType);
  unsigned short dstHS = dst->getHorzStride();

  if (rel == Rel_gt || srcElSize != defDstElSize ||
      (defInst->getSaturate() && srcType != defDstType) || inst->isRawMov() ||
      (dstType != defDstType &&
       (IS_FTYPE(defDstType) ||
        (IS_FTYPE(dstType) && defDstType != srcType)))) {
    unsigned short regOff = 0, subRegOff = 0;
    if (rel == Rel_gt) {
      // compute new dst for defInst
      // get dst portion based on src region
      unsigned defDstLB = defDstRegion->getLeftBound();

      unsigned srcLB = src->getLeftBound();
      const RegionDesc *srcRegionDesc = src->asSrcRegRegion()->getRegion();
      bool contRegion = srcRegionDesc->isContiguous(inst->getExecSize());

      uint32_t dist = defDstLB - srcLB, dstDist = 0, tempLen = 0;
      if (src->asSrcRegRegion()->isScalar() || contRegion) {
        // mov (1) V18(0,0)[1]:b 0x73:w [Align1]
        // mov (1) V18(0,1)[1]:b 0x61:w [Align1]
        // mov (1) V18(0,2)[1]:b 0x70:w [Align1]
        // mov (1) V18(0,3)[1]:b 0:w [Align1]
        // mov (1) V20(1,0)[1]:ud V18(0,0)[0;1,0]:ud [Align1]
        // length of subregoff part
        tempLen = dstRegion->getSubRegOff() * dstElSize + dist * dstHS;

        if (tempLen >= fg.builder->numEltPerGRF<Type_UB>()) {
          regOff = dst->getRegOff() + 1;
          subRegOff =
              (unsigned short)((tempLen - fg.builder->numEltPerGRF<Type_UB>()) /
                               defDstElSize);
        } else {
          regOff = dst->getRegOff();
          subRegOff = (unsigned short)tempLen / defDstElSize;
        }
      } else {
        // mov (16) V18(0,0)[1]:b 0x73:w [Align1]
        // mov (16) V18(0,16)[1]:b 0x61:w [Align1]
        // mov (16) V18(1,0)[1]:b 0x70:w [Align1]
        // mov (16) V18(1,16)[1]:b 0:w [Align1]
        // mov (32) V20(1,0)[1]:b V18(0,0)[32;16,1]:b [Align1]
        // mov (32) V20(2,0)[1]:b V18(0,16)[32;16,1]:b [Align1]

        // Compute the linear index of the first element from defInst's dst
        // in useInst's src.
        //
        // mov <2> V50(0, 14)<1>:b 0xa:w                  <- defInst
        // mov <16> V51(0, 9)<2>:b V50(0, 0)<0; 16, 2>:b  <- useInst
        //
        // Starting from left bound difference, dist = 14.
        //
        // FirstEltIndex is 7 = 14 / 2. With this index, we can compute
        // the register offset and sub-register offset in useInst's dst.
        //
        // In the above example, there is only a single row. In general
        // there may have multiple rows in useInst's src region.
        //
        // (1) convert difference in number of elements.
        vISA_ASSERT(dist % srcElSize == 0, "unexpected difference");
        dist = dist / srcElSize;

        // (2) compute row and column index, by default a single row.
        unsigned rowIndex = 0, colIndex = dist;
        if (srcRegionDesc->vertStride > 0) {
          rowIndex = dist / srcRegionDesc->vertStride;
          colIndex = dist % srcRegionDesc->vertStride;
        }

        // (3) compute the final linear index.
        vISA_ASSERT(srcRegionDesc->horzStride == 0 ||
                        colIndex % srcRegionDesc->horzStride == 0,
                    "invalid region");
        unsigned FirstEltIndex = rowIndex * srcRegionDesc->width +
                                 (srcRegionDesc->horzStride == 0
                                      ? colIndex
                                      : (colIndex / srcRegionDesc->horzStride));

        // (4) compute the register and subregister offet in useInst's dst.
        dstDist = FirstEltIndex * dstElSize * dstHS;
        tempLen = dstDist + dst->getSubRegOff() * dstElSize;
        regOff =
            (unsigned short)(dst->getRegOff() +
                             tempLen / fg.builder->numEltPerGRF<Type_UB>());

        subRegOff =
            (unsigned short)(tempLen % fg.builder->numEltPerGRF<Type_UB>()) /
            defDstElSize;
      }

      unsigned short defDstHS = defDstRegion->getHorzStride();
      if (!indirectDst) {
        newDstOpnd =
            fg.builder->createDst(dst->getBase(), regOff, subRegOff,
                                  dstHS * defDstHS, defDstRegion->getType());
      } else {
        newDstOpnd = fg.builder->createIndirectDst(
            dst->getBase(), dst->getSubRegOff(), dstHS * defDstHS,
            defDstRegion->getType(), (int16_t)dst->getAddrImm() + tempLen);
      }
    } else {
      unsigned char scale = dstElSize / defDstElSize;

      // If instruction that gets def hoisted is just re-interpretation of bits,
      // doesn't do conversion, then I think original type of the defInst
      // should be used. This preserves the original behavior.
      //
      // mov (8) V57(0,0)[1]:hf V52(0,0)[8;8,1]:f [Align1, Q1] %21
      // mov (8) V58(0,0)[1]:w  V59(0,0)[8;8,1]:w [Align1, Q1] %22
      if (dst->getType() == src->getType()) {
        if (!indirectDst) {
          newDstOpnd = fg.builder->createDst(
              dst->getBase(), dst->getRegOff(),
              (scale == 0 ? dst->getSubRegOff() / (defDstElSize / dstElSize)
                          : dst->getSubRegOff() * scale),
              dstHS, defDstRegion->getType());
        } else {
          newDstOpnd = fg.builder->createIndirectDst(
              dst->getBase(), dst->getSubRegOff(), dstHS,
              defDstRegion->getType(), dst->getAddrImm());
        }
      } else {
        if (!indirectDst) {
          newDstOpnd =
              fg.builder->createDst(dst->getBase(), dst->getRegOff(),
                                    dst->getSubRegOff(), dstHS, dst->getType());
        } else {
          newDstOpnd = fg.builder->createIndirectDst(
              dst->getBase(), dst->getSubRegOff(), dstHS, dst->getType(),
              dst->getAddrImm());
        }
      }
    }
  }
  return newDstOpnd;
}

//
//  inst0:   op0  rx<2>(0), ry0, ry1
//  inst1:   op1  rx<2>(1), ry2, ry3
//  inst2:   mov  rz<1>(0), rx<0, 0>(8; 8, 1)
//
// ==>
//
//  inst0:   op0, rz<2>(0), ry0, ry1
//  inst1:   op1, rz<2>(1), ry2, ry3
//  inst2:   mov  rz<1>(0), rx<0, 0>(8; 8, 1)  (to be deleted)
//
// Def-use/use-def chains will be updated as follows:
//
// (0) all use-defs remain the same for inst0 and inst1.
//
// (1) remove all use-defs of inst2. (They must be from inst0 and inst1,
//     which is the pre-condition of doHoisting.)
//
// (2) remove all def-uses of inst0 and inst1 from dst.
//
// (3) remove all def-uses of inst2.
//
// (4) add new def-uses to inst0 and inst1.
//
static void doHoisting(FlowGraph &fg, G4_BB *bb, INST_LIST_RITER revIter) {
  G4_INST *inst = *revIter;

  for (auto I = inst->def_begin(), E = inst->def_end(); I != E; ++I) {
    G4_INST *defInst = I->first;

    // Build a new dst operand for each def instruction.
    G4_DstRegRegion *newDst = buildNewDstOperand(fg, inst, defInst);

    // Update the defInst with a new operand and set attributes properly.
    if (inst->def_size() == 1) {
      defInst->setDest(newDst);
    } else {
      defInst->setDest(fg.builder->duplicateOperand(newDst)->asDstRegRegion());
    }

    // (4) for each def-use of inst, add it to defInst, if it is
    // an effective use.
    for (auto UI = inst->use_begin(), UE = inst->use_end(); UI != UE; ++UI) {
      G4_Operand *UseOpnd = UI->first->getOperand(UI->second);
      // this comparison is necessary, since uses of inst's dst may be
      // different from those from defInst's dst.
      G4_CmpRelation rel =
          defInst->getDst()->compareOperand(UseOpnd, *fg.builder);
      if (rel != Rel_disjoint) {
        defInst->addDefUse(UI->first, UI->second);
      }
    }

    if (inst->getPredicate()) {
      vISA_ASSERT(inst->def_size() == 1, "multiple defs not implemented");
      // Remove existing definitions on defInst[opnd_pred].
      defInst->removeDefUse(Opnd_pred);

      defInst->setPredicate(inst->getPredicate());

      // (4) Transfer definitions of inst[opnd_pred] to definitions of
      // defInst[opnd_pred].
      inst->transferDef(defInst, Opnd_pred, Opnd_pred);
    }
    if (inst->getSrc(0)->asSrcRegRegion()->isScalar() &&
        inst->getExecSize() > g4::SIMD1) {
      defInst->setExecSize(
          G4_ExecSize(defInst->getExecSize() * inst->getExecSize()));
    }
    defInst->setSaturate(inst->getSaturate() || defInst->getSaturate());
    if (!bb->isAllLaneActive()) {
      // set writeEnable of dstInst to be off
      defInst->setOptions((defInst->getOption() & ~0xFFF000C) |
                          (inst->getMaskOption()));
    }
  }

  // (1), (2), (3) Remove all defs/uses and it is ready to be deleted.
  inst->removeAllDefs();
  inst->removeAllUses();
}

void Optimizer::localDefHoisting() {
  unsigned numDefHoisted = 0;
  for (auto bb : fg) {
    for (auto I = bb->rbegin(); I != bb->rend(); /* empty */) {
      if (canHoist(fg, bb, I)) {
        doHoisting(fg, bb, I);
        ++numDefHoisted;

        // list::erase does not take a reverse_iterator.
        //
        // The base iterator is an iterator of the same type as the one
        // used to construct the reverse_iterator, but pointing to the
        // element next to the one that the reverse_iterator is currently
        // pointing to (a reverse_iterator has always an offset of -1
        // with respect to its base iterator).
        I = INST_LIST::reverse_iterator(bb->erase(--I.base()));
      } else {
        ++I;
      }
    }
  }

  VISA_DEBUG({
    std::cout
        << "             === Local Definition Hoisting Optimization ===\n";
    std::cout << "Number of defs hoisted: " << numDefHoisted << "\n";
  });
}

//
// Do very simple const only reassociation to fold const values
// e.g.,
// V2 = V1 + K1
// V3 = V2 + K2
// -->
// V3 = V1 + (K1 + K2)
// we only search one level (+ and +, * and *) for now as more complex
// reassociation should be taken care of by IGC earlier. also only do it for
// integer type for now
//
void Optimizer::reassociateConst() {
  for (auto BB : fg) {
    for (auto iter = BB->begin(), iterEnd = BB->end(); iter != iterEnd;
         ++iter) {
      G4_INST *inst = *iter;
      if (inst->opcode() != G4_add && inst->opcode() != G4_mul) {
        continue;
      }
      auto isSrc1Const = [](G4_INST *inst) {
        if (!IS_INT(inst->getDst()->getType())) {
          return false;
        }
        if (!inst->getSrc(0)->isImm() && inst->getSrc(1)->isImm()) {
          return true;
        } else if (inst->getSrc(0)->isImm() && !inst->getSrc(1)->isImm()) {
          inst->swapSrc(0, 1);
          inst->swapDefUse(); // swap def/use for src0 and src1
          return true;
        }
        return false;
      };
      if (!isSrc1Const(inst)) {
        continue;
      }
      auto src0Def = inst->getSingleDef(Opnd_src0);
      if (!src0Def) {
        continue;
      }

      auto isGoodSrc0Def = [isSrc1Const](G4_INST *def, G4_INST *use,
                                         const IR_Builder &builder) {
        vISA_ASSERT(use->getSrc(0)->isSrcRegRegion(),
                    "expect src0 to be src region");
        if (def->opcode() != use->opcode()) {
          return false;
        }
        if (def->getSaturate() || def->getPredicate() || def->getCondMod() ||
            def->getMaskOffset() != use->getMaskOffset()) {
          return false;
        }
        if (!isSrc1Const(def)) {
          return false;
        }
        auto useSrc = use->getSrc(0)->asSrcRegRegion();
        if (useSrc->hasModifier() ||
            def->getDst()->getTypeSize() != useSrc->getTypeSize() ||
            def->getDst()->compareOperand(useSrc, builder) != Rel_eq) {
          // make sure def fully defines use and have the same integer type size
          // (signed-ness should not matter)
          return false;
        }
        if (def->getDst()->compareOperand(def->getSrc(0), builder) !=
            Rel_disjoint) {
          // can't sink source if def overwrites it
          return false;
        }
        // additionally check for the use inst that dst type size is >= src type
        // size otherwise the first add may truncate upper bits due to overflow,
        // which makes reassociation unsafe
        if (useSrc->getTypeSize() < use->getDst()->getTypeSize()) {
          return false;
        }

        return true;
      };

      if (isGoodSrc0Def(src0Def, inst, builder) &&
          !chkFwdOutputHazard(src0Def, iter)) {
        // std::cout << "reassociate: \n";
        // src0Def->dump();
        // inst->dump();
        G4_Imm *constOne = src0Def->getSrc(1)->asImm();
        G4_Imm *constTwo = inst->getSrc(1)->asImm();
        G4_Imm *resultImm =
            builder.foldConstVal(constOne, constTwo, inst->opcode());

        if (resultImm) {
          inst->setSrc(builder.duplicateOperand(src0Def->getSrc(0)), 0);
          inst->setSrc(resultImm, 1);
          inst->removeDefUse(Opnd_src0);
          src0Def->copyDef(inst, Opnd_src0, Opnd_src0);
          // ToDo: remove this when DCE pass is enabled
          if (src0Def->use_size() == 0 &&
              !fg.globalOpndHT.isOpndGlobal(src0Def->getDst()) &&
              !src0Def->getDst()->isIndirect()) {
            src0Def->markDead();
            src0Def->removeAllDefs();
          }
          // std::cout << "--> new inst:\t";
          // inst->dump();
        }
      }
    }
    BB->erase(std::remove_if(BB->begin(), BB->end(),
                             [](G4_INST *inst) { return inst->isDead(); }),
              BB->end());
  }
}

static void hoistUseInst(G4_BB *bb, G4_INST *inst, INST_LIST_ITER forwardIter,
                         bool &canRemove) {
  // check if we can move the use inst up.
  // currently we do not handle multiple use for this optimization
  G4_INST *useInst = inst->use_front().first;
  if (inst->hasOneUse()) {
    forwardIter--;
    INST_LIST_ITER backwardIter = forwardIter;
    INST_LIST_ITER instListEnd = bb->end();
    while (backwardIter != instListEnd && *backwardIter != useInst) {
      backwardIter++;
    }

    INST_LIST_ITER useInstIter = backwardIter;
    backwardIter--;
    while (backwardIter != forwardIter) {
      if (useInst->isWAWdep(*backwardIter) ||
          useInst->isRAWdep(*backwardIter) ||
          useInst->isWARdep(*backwardIter)) {
        break;
      }
      backwardIter--;
    }
    if (backwardIter != forwardIter) {
      canRemove = false;
    } else {
      // hoisting
      backwardIter++;
      bb->insertBefore(backwardIter, useInst);
      bb->erase(useInstIter);
    }
  } else {
    canRemove = false;
  }
}

// conver modifier(imm)
template <class T>
static typename std::enable_if<std::is_floating_point<T>::value, T>::type
getImmValue(T imm, G4_SrcModifier modifier) {
  switch (modifier) {
  case Mod_Minus:
    return -imm;
  case Mod_Abs:
    return std::abs(imm);
  case Mod_Minus_Abs:
    return -(std::abs(imm));
  case Mod_Not:
    vISA_ASSERT_UNREACHABLE("unexpected not modifier for floating types");
    return imm;
  default:
    return imm;
  }
}

template <class T>
static typename std::enable_if<std::is_integral<T>::value, T>::type
getImmValue(T imm, G4_SrcModifier modifier) {
  switch (modifier) {
  case Mod_Minus:
    return -imm;
  case Mod_Abs:
    return std::llabs(imm);
  case Mod_Minus_Abs:
    return -(std::llabs(imm));
  case Mod_Not:
    return ~imm;
  default:
    return imm;
  }
}

// Source operand of the MOV instruction is already known being able to be
// propagated into all its uses. But, due to the dependency issue, it cannot be
// propagated. Try to propagate the type if a narrower type could be used.
static bool propagateType(IR_Builder &Builder, G4_BB *BB, G4_INST *Mov,
                          G4_INST::MovType MT) {
  // Only propagate type if a narrower type could be used.
  if (MT != G4_INST::ZExt && MT != G4_INST::SExt)
    return false;

  G4_DstRegRegion *Dst = Mov->getDst();
  if (Dst->isIndirect())
    return false;

  // Check all propagation types are the same.
  G4_Type PT = Type_UNDEF;
  for (auto UI = Mov->use_begin(), UE = Mov->use_end(); UI != UE; ++UI) {
    auto Use = UI->first;
    auto OpndNum = UI->second;
    auto Opnd = Use->getOperand(OpndNum);
    if (!Opnd->isSrcRegRegion())
      return false;
    if (Opnd->asSrcRegRegion()->isIndirect())
      return false;
    G4_Type PropType = Use->getPropType(OpndNum, MT, Mov);
    if (PropType == Type_UNDEF)
      return false;
    if (PT != Type_UNDEF && PT != PropType)
      return false;
    PT = PropType;
  }
  if (PT == Type_UNDEF)
    return false;
  // Create a new destination of MOV of the propagation type.
  // Consider both execution size and the dst horizontal stride to calculate
  // the number of elements needed, so that we have the enough var size when
  // creating the temp var.
  unsigned NumElt = Mov->getExecSize() * Dst->getHorzStride();
  auto NewDcl = Builder.createTempVar(NumElt, PT, Any);
  auto NewDst = Builder.createDstRegRegion(NewDcl, Dst->getHorzStride());
  Mov->setDest(NewDst);
  // Propagate type
  for (auto UI = Mov->use_begin(), UE = Mov->use_end(); UI != UE; ++UI) {
    auto Use = UI->first;
    auto OpndNum = UI->second;
    auto Opnd = Use->getOperand(OpndNum)->asSrcRegRegion();
    auto NewOpnd = Builder.createSrcRegRegion(NewDcl, Opnd->getRegion());
    Use->setSrc(NewOpnd, OpndNum - 1);
  }
  return true;
}

static unsigned getMaskSize(G4_INST *Inst, Gen4_Operand_Number OpNum) {
  G4_Operand *Opnd = Inst->getOperand(OpNum);
  vISA_ASSERT(Opnd, "null opnd");

  if (Opnd) {
    G4_Declare *Dcl = Opnd->getTopDcl();
    if (Dcl == nullptr) {
      // There is no top declaration for this operand, so this is ARF.
      return 32;
    }
    return Dcl->getRegVar()->isFlag() ? Dcl->getNumberFlagElements()
                                      : Dcl->getByteSize();
  }

  return 0;
}

void Optimizer::removePartialMovs() {
  auto IsValidCandidate = [](G4_Operand *dst, G4_Operand *src, int execSize) {
    if (dst->isDstRegRegion() && src->isSrcRegRegion()) {
      unsigned short dstSize, sourceSize;
      dstSize =
          dst->getTopDcl()->getTotalElems() * dst->getTopDcl()->getElemSize();
      sourceSize =
          src->getTopDcl()->getTotalElems() * src->getTopDcl()->getElemSize();

      if (!src->asSrcRegRegion()->getRegion()->isSingleStride(execSize)) {
        return false;
      }
      if (dst->asDstRegRegion()->getHorzStride() != 1 && execSize != 1) {
        return false;
      }
      if (src->getRightBound() - src->getLeftBound() !=
          dst->getRightBound() - dst->getLeftBound()) {
        return false;
      }
      if (dstSize != sourceSize) {
        return false;
      }
      return true;
    }

    // Common cases should be covered.
    return false;
  };

  auto IsSameDstSrc = [](G4_Operand *dst, G4_Operand *src) {
    if (dst->isDstRegRegion() && src->isSrcRegRegion()) {
      if (dst->getTopDcl() != src->getTopDcl()) {
        return false;
      }

      unsigned short dstSize, sourceSize;
      dstSize =
          dst->getTopDcl()->getTotalElems() * dst->getTopDcl()->getElemSize();
      sourceSize =
          src->getTopDcl()->getTotalElems() * src->getTopDcl()->getElemSize();

      if (dst->asDstRegRegion()->getHorzStride() != 1) {
        return false;
      }
      if (src->getRightBound() - src->getLeftBound() !=
          dst->getRightBound() - dst->getLeftBound()) {
        return false;
      }
      if (dstSize != sourceSize) {
        return false;
      }
      return true;
    }

    // Common cases should be covered.
    return false;
  };

  auto IsStatelessSend = [](G4_INST *inst) {
    if (!inst->isSend()) {
      return false;
    }
    auto msgDesc = inst->asSendInst()->getMsgDesc();

    if (!msgDesc->isLSC() || msgDesc->isSLM() || !inst->getMsgDescRaw()) {
      return false;
    }

    if (inst->getMsgDescRaw()) {
      uint32_t desc = inst->getMsgDescRaw()->getDesc();

      if ((desc >> 29) & 0x3) {
        return false;
      }
    }

    return true;
  };

  for (G4_BB *bb : fg) {
    bb->resetLocalIds();

    INST_LIST_ITER ii = bb->begin(), iend(bb->end());
    while (ii != iend) {
      INST_LIST_ITER firstIt = ii;
      G4_INST *inst1 = *ii;
      G4_Operand *dst1 = inst1->getDst();
      G4_Operand *src1 = inst1->getSrc(0);
      if (inst1->opcode() != G4_mov || !dst1) {
        ii++;
        continue;
      }
      ii++;
      if (ii == iend) {
        break;
      }
      INST_LIST_ITER secondIt = ii;
      G4_INST *inst2 = *ii;
      G4_Operand *dst2 = inst2->getDst();
      G4_Operand *src2 = inst2->getSrc(0);
      if (inst2->opcode() != G4_mov || !dst2) {
        continue;
      }
      ii++;
      if (ii == iend) {
        break;
      }
      INST_LIST_ITER thirdIt = ii;
      G4_INST *inst3 = *ii;
      G4_Operand *dst3 = inst3->getDst();
      G4_Operand *src3 = inst3->getSrc(0);
      if (ii == iend) {
        break;
      }
      if (inst3->opcode() != G4_mov || !dst3) {
        continue;
      }

      if (inst1->getDst()->getTopDcl()->getRegFile() != G4_GRF ||
          inst2->getDst()->getTopDcl()->getRegFile() != G4_GRF ||
          inst3->getDst()->getTopDcl()->getRegFile() != G4_GRF) {
        continue;
      }

      // All three instructions can be propagated
      G4_INST::MovType MT1 = inst1->canPropagate();
      G4_INST::MovType MT2 = inst2->canPropagate();
      G4_INST::MovType MT3 = inst3->canPropagate();
      if (MT1 == G4_INST::SuperMov || MT2 == G4_INST::SuperMov ||
          MT3 == G4_INST::SuperMov) {
        continue;
      }

      // Constraints for each instruction
      if (!IsValidCandidate(dst1, src1, inst1->getExecSize()) ||
          !IsValidCandidate(dst2, src2, inst2->getExecSize()) ||
          !IsValidCandidate(dst3, src3, inst3->getExecSize())) {
        continue;
      }

      // Profitable
      if (!isCopyPropProfitable(inst1) || !isCopyPropProfitable(inst2) ||
          !isCopyPropProfitable(inst3)) {
        continue;
      }

      // Same declare in both dst and src for inst1 and inst2
      if (src1->getTopDcl() != src2->getTopDcl() ||
          dst1->getTopDcl() != dst2->getTopDcl()) {
        continue;
      }

      // Used in same single instruction inst3
      if (inst1->use_size() != 1 || inst2->use_size() != 1 ||
          inst3->use_size() != 1 ||
          inst1->use_begin()->first != inst2->use_begin()->first ||
          inst1->def_begin()->first != inst2->def_begin()->first ||
          inst1->use_begin()->first != inst3) {
        continue;
      }

      // Same mask order, to avoid the reverting
      BitSet srcMask(getMaskSize(inst1, Opnd_src0), 0);
      BitSet dstMask(getMaskSize(inst1, Opnd_src0), 0);
      src1->updateFootPrint(srcMask, true, builder);
      dst1->updateFootPrint(dstMask, true, builder);
      if (dstMask != srcMask) {
        continue;
      }
      src2->updateFootPrint(srcMask, true, builder);
      dst2->updateFootPrint(dstMask, true, builder);
      if (dstMask != srcMask) {
        continue;
      }

      // Check if use can be propgated.
      G4_INST *useInst = inst3->use_begin()->first;
      Gen4_Operand_Number opndNum = inst3->use_begin()->second;
      if (!inst3->canPropagateTo(
              useInst, opndNum, MT3, !bb->isAllLaneActive(),
              IsStatelessSend(useInst) &&
                  IsSameDstSrc(inst3->getDst(),
                               useInst->getSrc(opndNum - 1)))) {
        continue;
      }

      // Propgation for the define to use
      G4_INST *defInst = inst1->def_begin()->first;
      G4_Operand *useSrc = useInst->getSrc(opndNum - 1);
      G4_Operand *new_src_opnd = builder.createSrcRegRegion(
          Mod_src_undef, src1->asSrcRegRegion()->getRegAccess(),
          src1->asSrcRegRegion()->getBase(),
          src1->asSrcRegRegion()->getRegOff(),
          src1->asSrcRegRegion()->getSubRegOff(),
          useSrc->asSrcRegRegion()->getRegion(), useSrc->getType());
      useInst->setSrc(new_src_opnd, opndNum - 1);
      inst1->copyDefsTo(useInst, true);
      inst3->copyUsesTo(defInst, true);

      inst1->removeAllDefs();
      inst1->removeAllUses();
      inst2->removeAllDefs();
      inst2->removeAllUses();
      inst3->removeAllDefs();
      inst3->removeAllUses();

      ii++;
      bb->erase(firstIt);
      bb->erase(secondIt);
      bb->erase(thirdIt);
    }
  }
}

void Optimizer::localCopyPropagation() {
  for (G4_BB *bb : fg) {
    bb->resetLocalIds();

    INST_LIST_ITER ii = bb->begin(), iend(bb->end());
    while (ii != iend) {
      G4_INST *inst = *ii;
      G4_Operand *dst = inst->getDst();
      if (!dst) {
        ii++;
        continue;
      }

      builder.doConsFolding(inst);
      inst = builder.doMathConsFolding(ii);
      builder.doSimplification(inst);

      G4_INST::MovType MT = inst->canPropagate();
      // Skip super mov.
      if (MT == G4_INST::SuperMov) {
        ii++;
        continue;
      }

      if (!isCopyPropProfitable(inst)) {
        ++ii;
        continue;
      }
      bool canRemove = true;

      // check if each use may be copy propagated.
      USE_EDGE_LIST_ITER iter, iend1(inst->use_end());
      for (iter = inst->use_begin(); iter != iend1; iter++) {
        G4_INST *useInst = iter->first;
        Gen4_Operand_Number opndNum = iter->second;

        if (inst->getDst()->isDirectA0() && useInst->isSplitSend() &&
            VISA_WA_CHECK(builder.getPWaTable(),
                          WaSendSEnableIndirectMsgDesc)) {
          canRemove = false;
          break;
        }

        if (!inst->canPropagateTo(useInst, opndNum, MT,
                                  !bb->isAllLaneActive())) {
          canRemove = false;
          break;
        }

        // Make sure there is no lifetime.end for src0 of the move inst
        INST_LIST_ITER cpIter = ii;
        cpIter++;
        while (*cpIter != useInst) {
          // Detect patterns like:
          //
          // mov A, B
          // ...
          // lifetime.end B
          // op C, A, D
          //
          // Because of presence of lifetime.end B, copy propagation for inst
          // mov A, B
          // cannot be done

          if ((*cpIter)->isLifeTimeEnd()) {
            // Check whether lifetime end is for same opnd
            G4_Declare *lifetimeEndTopDcl =
                GetTopDclFromRegRegion((*cpIter)->getSrc(0));
            G4_Declare *curInstDstTopDcl =
                GetTopDclFromRegRegion((*ii)->getDst());

            if (lifetimeEndTopDcl == curInstDstTopDcl) {
              canRemove = false;
              break;
            }
          }

          //
          // Following instructions may use acc0 register in HWComformity,
          // if any of them appear in the propagation range, it may have
          // correctness issue.
          //  Such as in following case:
          // addc
          // mov  V1, acc0
          // mulh
          // add   V2, V1, V3
          // Since HW conformity will replace mulh with mul + acc0 dst,
          // the propagation acc0 through V1 will introduce correctness
          // issue.
          //
          if (inst->getSrc(0)->isAccReg() &&
              ((*cpIter)->opcode() == G4_mul ||
               (*cpIter)->opcode() == G4_mulh ||
               (*cpIter)->opcode() == G4_pln ||
               (*cpIter)->opcode() == G4_pseudo_sada2 ||
               (*cpIter)->opcode() == G4_madw ||
               (*cpIter)->opcode() == G4_subb ||
               (*cpIter)->opcode() == G4_addc)) {
            canRemove = false;
            break;
          }

          cpIter++;
        }
      } // for uses

      if (canRemove && inst->getSrc(0)->isSrcRegRegion()) {
        // check for anti-dependencies for src0 of the move instruction
        bool def_use_in_between = false;

        G4_INST *lastUse = inst->use_front().first;
        for (USE_EDGE_LIST_ITER iter = inst->use_begin(),
                                uend = inst->use_end();
             iter != uend; ++iter) {
          G4_INST *useInst = iter->first;
          if (useInst->getLocalId() > lastUse->getLocalId()) {
            lastUse = useInst;
          }
        }

        INST_LIST_ITER forwardIter = ii;
        forwardIter++;
        INST_LIST_ITER instListEnd = bb->end();

        while (!def_use_in_between && forwardIter != instListEnd &&
               *forwardIter != lastUse) {
          if ((*forwardIter)->isWARdep(inst)) {
            def_use_in_between = true;
            break;
          }
          forwardIter++;
        }

        // check if hoisting is possible
        if (def_use_in_between) {
          hoistUseInst(bb, inst, forwardIter, canRemove);
        }

        if (!canRemove) {
          // Check whether the type could be propagated instead to demote
          // type if possible.
          propagateType(builder, bb, inst, MT);
        }
      }

      if (!canRemove) {
        ii++;
        continue;
      }

      G4_Operand *src = inst->getSrc(0);
      // do propagation
      for (iter = inst->use_begin(); iter != iend1; /* empty */) {
        G4_INST *useInst = (*iter).first;
        Gen4_Operand_Number opndNum = (*iter).second;
        G4_Operand *use = useInst->getOperand(opndNum);
        G4_Type propType = useInst->getPropType(opndNum, MT, inst);

        // replace use with def
        if (src->isImm()) {
          auto newImmVal =
              G4_Imm::typecastVals(src->asImm()->getImm(), propType);
          G4_Imm *newImm = builder.createImm(newImmVal, propType);
          G4_SrcModifier modifier = use->asSrcRegRegion()->getModifier();
          if (modifier != Mod_src_undef) {
            if (IS_TYPE_FLOAT_ALL(propType)) {
              if (propType == Type_DF) {
                double imm = getImmValue(newImm->getDouble(), modifier);
                newImm = builder.createDFImm(imm);
              } else {
                float imm = getImmValue(newImm->getFloat(), modifier);
                newImm = builder.createImm(imm);
              }
            } else {
              int64_t imm = getImmValue(newImm->getImm(), modifier);
              newImm = builder.createImm(imm, propType);
            }
          }
          useInst->setSrc(newImm, opndNum - 1);
        } else {
          if (use == NULL) {
            break;
          }
          G4_SrcModifier new_mod = mergeModifier(src, use);

          unsigned use_elsize = use->getTypeSize();
          unsigned dstElSize = inst->getDst()->getTypeSize();
          const RegionDesc *rd = src->asSrcRegRegion()->getRegion();
          G4_Operand *new_src_opnd = NULL;
          bool new_src = false;
          unsigned char scale = 1, newExecSize = useInst->getExecSize();

          // Compute the composed region if exists.
          auto getComposedRegion =
              [this](unsigned dStride, unsigned ex1, const RegionDesc *rd1,
                  unsigned ex2, const RegionDesc *rd2) -> const RegionDesc * {
            // Easy cases.
            if (rd1->isScalar())
              return rd1;
            else if (rd2->isScalar())
              return rd2;
            else if (dStride == 1 && rd1->isContiguous(ex1))
              return rd2;
            else if (dStride == 1 && rd2->isContiguous(ex2))
              return rd1;

            // rd1 and rd2 must be single strided. Use a non-zero
            // invalid stride value as the initial value, which
            // simplifies and unifies the checking.
            uint16_t stride1 = 64;
            if (rd1->isContiguous(ex1))
              stride1 = 1;
            else
              rd1->isSingleNonUnitStride(ex1, stride1);

            uint16_t stride2 = 64;
            if (rd2->isContiguous(ex2))
              stride2 = 1;
            else
              rd2->isSingleNonUnitStride(ex2, stride2);

            // All are single strided; the composition is the product of
            // strides.
            if (stride1 * stride2 * dStride <= 32)
              return builder.createRegionDesc(
                  (uint16_t)ex2, stride1 * stride2 * dStride, 1, 0);

            // Should be unreachable, since the legality check
            // before should reject cases that are difficult to do
            // composition. Assert?
            return nullptr;
          };

          if (MT == G4_INST::Trunc) {
            G4_DstRegRegion *dst = inst->getDst();
            G4_SrcRegRegion *src0 = src->asSrcRegRegion();
            unsigned typeSizeRatio = src0->getTypeSize() / dst->getTypeSize();
            unsigned numElt =
                src0->isScalar() ? 1 : inst->getExecSize() * typeSizeRatio;
            // src0 region is guaranteed to be scalar/contiguous due to
            // canPropagate() check earlier
            const RegionDesc *region =
                src0->isScalar()
                    ? builder.getRegionScalar()
                    : builder.createRegionDesc(
                          useInst->getExecSize(),
                          (uint16_t)inst->getExecSize() * typeSizeRatio,
                          inst->getExecSize(), (uint16_t)typeSizeRatio);
            if (src0->isIndirect()) {
              new_src_opnd = builder.createIndirectSrc(
                  new_mod, src0->getBase(), src0->getRegOff(),
                  src0->getSubRegOff() * typeSizeRatio, region, propType,
                  src0->getAddrImm());
            } else {
              G4_Declare *newDcl =
                  builder.createTempVar(numElt, inst->getDst()->getType(), Any);
              newDcl->setAliasDeclare(src0->getBase()->asRegVar()->getDeclare(),
                                      0);

              new_src_opnd = builder.createSrcRegRegion(
                  new_mod, Direct, newDcl->getRegVar(), src0->getRegOff(),
                  src0->getSubRegOff() * typeSizeRatio, region, propType);
            }
            new_src = true;
          } else if (dstElSize < use_elsize) {
            // FIXME: How could this happen? Revisit later if
            // NoMask is guaranteed.
            // TODO!!! src is aligned to use type. -- should check this.
            new_src = true;
            scale = use_elsize / dstElSize;
            unsigned short vs = rd->vertStride, wd = rd->width;
            // packed word/byte
            if (use->asSrcRegRegion()->isScalar()) {
              rd = builder.getRegionScalar();
            } else if (inst->isComprInst() && vs == wd) {
              rd = builder.getRegionStride1();
            } else {
              rd = builder.createRegionDesc(vs / scale, wd / scale, 1);
            }
          } else if (inst->getExecSize() < useInst->getExecSize() && rd &&
                     use->isSrcRegRegion()) {
            unsigned dStride = inst->getDst()->getHorzStride();
            const RegionDesc *rd2 = use->asSrcRegRegion()->getRegion();
            if (auto compRd = getComposedRegion(dStride, inst->getExecSize(),
                                                rd, newExecSize, rd2)) {
              new_src = true;
              rd = compRd;
            }
          }

          if (new_mod != Mod_src_undef || new_src) {
            // For truncation case, new src operand is already built.
            if (MT != G4_INST::Trunc) {
              new_src_opnd = builder.createSrcRegRegion(
                  new_mod, src->asSrcRegRegion()->getRegAccess(),
                  src->asSrcRegRegion()->getBase(),
                  src->asSrcRegRegion()->getRegOff(),
                  src->asSrcRegRegion()->getSubRegOff() / scale, rd, propType);
              if (src->asSrcRegRegion()->getRegAccess() != Direct) {
                new_src_opnd->asSrcRegRegion()->setImmAddrOff(
                    src->asSrcRegRegion()->getAddrImm());
              }
            }
          } else {
            new_src_opnd = builder.duplicateOperand(src);
            new_src_opnd->asSrcRegRegion()->setModifier(new_mod);
            new_src_opnd->asSrcRegRegion()->setType(builder, propType);
          }
          useInst->setSrc(new_src_opnd, opndNum - 1);
        }

        iter = inst->eraseUse(iter);
        // due to truncation a (partial) def of the move may no longer be a def
        // of the use
        inst->copyDef(useInst, Opnd_src0, opndNum, true);

        builder.doConsFolding(useInst);
      }
      // remove decl corresponding to this def
      // TODO!!! what if there is some alias to this decl?
      // remove MOV inst

      // remove it from the use list of its deflists
      inst->removeDefUse(Opnd_src0);

      INST_LIST_ITER tmp = ii;
      ii++;
      bb->erase(tmp);
    }
  }
}

void Optimizer::localInstCombine() { InstCombine(builder, fg); }

void Optimizer::cselPeepHoleOpt() {
  if (!builder.hasCondModForTernary()) {
    return;
  }
  G4_SrcRegRegion *cmpSrc0 = NULL;
  G4_Operand *cmpSrc1 = NULL;
  for (G4_BB *bb : fg) {
    INST_LIST_ITER ii;
    INST_LIST_ITER nextIter;
    INST_LIST_ITER iiEnd;
    if (bb->empty()) {
      continue;
    }

    bb->resetLocalIds();
    ii = bb->begin();
    iiEnd = bb->end();

    nextIter = ii;

    do {
      ii = nextIter;
      ++nextIter;
      G4_INST *inst = *ii;
      G4_opcode op = inst->opcode();
      bool hasGRFDst = inst->getDst() && !inst->hasNULLDst();
      /*
      csel doesn't have the same semantics for destination
      as cmp instruction
      */
      if (op != G4_cmp || hasGRFDst || inst->getPredicate() || inst->isDead() ||
          !inst->getSrc(0)->isSrcRegRegion()) {
        continue;
      }

      cmpSrc0 = inst->getSrc(0)->asSrcRegRegion();
      cmpSrc1 = inst->getSrc(1);

      G4_CondMod *cModifier = inst->getCondMod();

      // check if dst is global
      if (fg.globalOpndHT.isOpndGlobal(cModifier)) {
        continue;
      }

      /*
      csel instruction implicitly compares src2 to 0
      only supports floats
      no predication
      */

      if (!cmpSrc1->isImm() ||
          (cmpSrc1->asImm()->getImm() != 0 &&
           (cmpSrc1->asImm()->getType() != Type_F ||
            cmpSrc1->asImm()->getFloat() != -0.0f)) ||
          cmpSrc0->getType() != Type_F || cmpSrc0->isImm())
        continue;

      if (inst->getSrc(0)->isRelocImm() || inst->getSrc(1)->isRelocImm()) {
        continue;
      }

      // Only allow single strided regions.
      uint16_t src0Stride = 0;
      if (!cmpSrc0->getRegion()->isSingleStride(inst->getExecSize(),
                                                src0Stride))
        continue;

      /*
          Can do scan until use instruction to see if src0 is modified
          but I think for general case this will suffice. If we are
          not capturing opportunities can revisit.
      */

      if (inst->useEmpty())
        continue;

      int execSize = inst->getExecSize();
      if (execSize == 2)
        continue;

      USE_EDGE_LIST_ITER iter = inst->use_begin();
      USE_EDGE_LIST_ITER endUseList = inst->use_end();

      bool canOpt = true;
      int maxInstID = 0;

      for (; iter != endUseList; ++iter) {
        G4_INST *useInst = (*iter).first;

        if (useInst->getNumSrc() != 2) {
          canOpt = false;
          break;
        }

        maxInstID = std::max(useInst->getLocalId(), maxInstID);
        G4_Operand *dstUse = useInst->getDst();
        G4_Operand *selSrc0 = useInst->getSrc(0);
        G4_Operand *selSrc1 = useInst->getSrc(1);

        if (useInst->opcode() != G4_sel || selSrc0->isImm() ||
            selSrc1->isImm() || selSrc0->getType() != Type_F ||
            selSrc1->getType() != Type_F || dstUse->getType() != Type_F ||
            // 3-src restriction
            !builder.tryToAlignOperand(dstUse, 16) ||
            !builder.tryToAlignOperand(selSrc0, 16) ||
            !builder.tryToAlignOperand(selSrc1, 16)) {
          canOpt = false;
          break;
        }

        //   if inst is NoMask use inst can be anything.
        //   if inst is not NoMask then useInst needs to be subset of inst.
        if (!(inst->getMaskOption() & InstOpt_WriteEnable)) {
          auto isInclusive = [](int lb1, int rb1, int lb2, int rb2) {
            return lb1 <= lb2 && rb1 >= rb2;
          };
          if (!isInclusive(inst->getMaskOffset(),
                           inst->getMaskOffset() + inst->getExecSize(),
                           useInst->getMaskOffset(),
                           useInst->getMaskOffset() + useInst->getExecSize())) {
            canOpt = false;
            break;
          }
        }

        uint8_t numPredDefs = 0;
        DEF_EDGE_LIST_ITER useIter = useInst->def_begin();
        DEF_EDGE_LIST_ITER iterEnd = useInst->def_end();

        //    Just in case some weird code is generated with partial writes to
        //    predicate
        for (; useIter != iterEnd; ++useIter) {
          if ((*useIter).second == Opnd_pred)
            ++numPredDefs;

          // Check whether pseudo_kill for dst exists between cmp and sel
          // cmp.xx.fx.0 (8) ... src0 $0
          // ...
          // pseudo_kill dst
          // (f0) sel (8) dst src1 src2
          //
          // These two cannot be merged because pseudo_kill is in between them

          INST_LIST_ITER cselOptIt = ii;
          cselOptIt++;
          while ((*cselOptIt) != useInst) {
            if ((*cselOptIt)->isLifeTimeEnd()) {
              if (GetTopDclFromRegRegion((*cselOptIt)->getDst()) ==
                  GetTopDclFromRegRegion(useInst->getDst())) {
                canOpt = false;
                break;
              }
            }

            cselOptIt++;
          }
        }

        if (numPredDefs > 1) {
          canOpt = false;
          break;
        }
      }

      INST_LIST_ITER tempInstIter = nextIter;
      // explicit check that cmp sr0 is not over written or partially writen to
      // between cmp and sel.
      for (; tempInstIter != iiEnd; ++tempInstIter) {
        G4_INST *tempInst = *tempInstIter;

        if (tempInst->getLocalId() == maxInstID) {
          break;
        }

        if (!tempInst->getDst())
          continue;

        // also checks for indirect, will return inerference.
        G4_CmpRelation rel =
            tempInst->getDst()->compareOperand(cmpSrc0, builder);
        if (rel != Rel_disjoint) {
          canOpt = false;
          break;
        }
      }

      if (canOpt) {
        for (auto iter = inst->use_begin(); iter != inst->use_end();
             /*empty*/) {
          G4_INST *useInst = (*iter).first;
          G4_CondMod *mod = inst->getCondMod();
          useInst->setOpcode(G4_csel);
          useInst->setSrc(builder.duplicateOperand(inst->getSrc(0)), 2);
          useInst->setCondMod(builder.duplicateOperand(mod));
          useInst->setPredicate(NULL);

          G4_SrcRegRegion *opnd2 = useInst->getSrc(2)->asSrcRegRegion();

          if (!opnd2->isScalar() &&
              inst->getExecSize() > useInst->getExecSize()) {
            // earlier check establishes that useInst mask is equivalent or
            // subset sel instruction
            /*
                case which considering:
                cmp (16)
                sel (8)
            */
            if (useInst->getMaskOffset() != inst->getMaskOffset()) {
              // check elsewhere guarantees this is float.
              G4_Type type = opnd2->getType();
              unsigned short typeSize = TypeSize(type);
              unsigned offset =
                  opnd2->getRegOff() * kernel.numEltPerGRF<Type_UB>() +
                  opnd2->getSubRegOff() * typeSize;
              offset += useInst->getExecSize() * src0Stride * typeSize;

              auto newSrc2 = builder.createSrcRegRegion(
                  opnd2->getModifier(), Direct, opnd2->getBase(),
                  offset / kernel.numEltPerGRF<Type_UB>(),
                  (offset % kernel.numEltPerGRF<Type_UB>()) / typeSize,
                  opnd2->getRegion(), opnd2->getType());
              useInst->setSrc(newSrc2, 2);
            }
          }
          //
          // Modifying useDef links
          //
          // cmp.xx.f0.0 (8) ... src2 $0  <- inst (to be deleted)
          // (f0) sel (8) dst src0 src1   <- useInst
          // =>
          // csel.xx.f0.0 (8) dst src0 src1 src2

          // useInst's predicate becomes NULL.
          iter = inst->eraseUse(iter);

          // inst's src0 becomes useInst's src2.
          inst->copyDef(useInst, Opnd_src0, Opnd_src2);
        }
        vISA_ASSERT(inst->useEmpty(), "all predicate uses are removed.");
        inst->removeAllDefs();
        bb->erase(ii);
      }
    } while (nextIter != iiEnd);
  }
}

// helper function to convert
// and/or p3 p1 p2
// ==>
// (p1) sel t1 1 0
// (p2) sel t2 1 0
// and/or.nz.p3 t1 t2
// if the original inst is NoMask and Q1/H1, we do
// and/or p3 p1 p2
// ==>
// and/or (1) p3 p1 p2
// p3's type is uw for simd8/16 and ud for simd32
static void expandPseudoLogic(IR_Builder &builder, G4_BB *bb,
                              INST_LIST_ITER &iter)

{
  G4_INST *inst = *iter;
  vISA_ASSERT(inst->isPseudoLogic(),
              "inst must be either pseudo_and/or/xor/not");
  INST_LIST_ITER newIter = iter;

  bool isFirstInst = iter == bb->begin();
  if (!isFirstInst) {
    --iter;
  }

  auto canFoldOnSIMD1 = [=, &builder]() {
    if (inst->isWriteEnableInst() &&
        (inst->getMaskOffset() == 0 || inst->getMaskOffset() == 16) &&
        // we can't do this for simd8 inst in simd16 kernels as it will
        // overwrite upper flag bits
        (inst->getExecSize() > g4::SIMD8 ||
         inst->getExecSize() == builder.kernel.getSimdSize())) {
      return true;
    }

    // inst writes the whole flag.
    if (inst->isWriteEnableInst() && inst->getMaskOffset() == 0) {
      auto Dcl = inst->getDst()->getTopDcl();
      if (Dcl && Dcl->getNumberFlagElements() <= inst->getExecSize()) {
        return true;
      }
    }

    return false;
  };

  if (canFoldOnSIMD1()) {
    G4_opcode newOpcode = G4_illegal;
    if (inst->getMaskOffset() == 16) {
      vISA_ASSERT(inst->getExecSize() == g4::SIMD16,
                  "Only support simd16 pseudo-logic instructions");
      // we have to use the upper flag bits (.1) instead
      vISA_ASSERT(inst->getSrc(0)->isSrcRegRegion() &&
                      inst->getSrc(0)->isFlag(),
                  "expect src0 to be flag");
      auto newSrc0 = builder.createSrcWithNewSubRegOff(
          inst->getSrc(0)->asSrcRegRegion(), 1);
      inst->setSrc(newSrc0, 0);
      if (inst->getSrc(1) != nullptr) {
        vISA_ASSERT(inst->getSrc(1)->isSrcRegRegion() &&
                        inst->getSrc(1)->isFlag(),
                    "expect src1 to be flag");
        auto newSrc1 = builder.createSrcWithNewSubRegOff(
            inst->getSrc(1)->asSrcRegRegion(), 1);
        inst->setSrc(newSrc1, 1);
      }
      auto newDst = builder.createDstWithNewSubRegOff(inst->getDst(), 1);
      inst->setDest(newDst);
    }

    switch (inst->opcode()) {
    case G4_pseudo_and:
      newOpcode = G4_and;
      break;
    case G4_pseudo_or:
      newOpcode = G4_or;
      break;
    case G4_pseudo_xor:
      newOpcode = G4_xor;
      break;
    case G4_pseudo_not:
      newOpcode = G4_not;
      break;
    default:
      vISA_ASSERT_UNREACHABLE(
          "unexpected opcode for pseudo-logic instructions");
    }

    inst->setOpcode(newOpcode);
    inst->setExecSize(g4::SIMD1);
  } else {
    G4_ExecSize tmpSize = inst->getExecSize();
    auto LowerOpnd = [=, &builder](Gen4_Operand_Number opNum,
                                   G4_INST *&SI) -> G4_Operand * {
      G4_Operand *Opnd = inst->getOperand(opNum);
      if (Opnd) {
        auto src = Opnd->asSrcRegRegion();
        auto newDcl = builder.createTempVar(tmpSize, Type_UW, Any);
        auto newDst = builder.createDst(newDcl->getRegVar(), 0, 0, 1, Type_UW);
        auto newPred = builder.createPredicate(PredState_Plus, src->getBase(),
                                               src->getSubRegOff());
        auto newSel = builder.createInternalInst(
            newPred, G4_sel, nullptr, g4::NOSAT, tmpSize, newDst,
            builder.createImm(1, Type_UW), builder.createImm(0, Type_UW),
            inst->getOption());
        inst->transferDef(newSel, opNum, Gen4_Operand_Number::Opnd_pred);
        bb->insertBefore(newIter, newSel);
        SI = newSel;
        const RegionDesc *rd = tmpSize == g4::SIMD1
                                   ? builder.getRegionScalar()
                                   : builder.getRegionStride1();
        return builder.createSrcRegRegion(newDcl, rd);
      }
      return Opnd;
    };

    G4_INST *Sel0 = nullptr;
    G4_Operand *logicSrc0 = LowerOpnd(Gen4_Operand_Number::Opnd_src0, Sel0);

    G4_INST *Sel1 = nullptr;
    G4_Operand *logicSrc1 = LowerOpnd(Gen4_Operand_Number::Opnd_src1, Sel1);

    if (logicSrc1 == nullptr) {
      vISA_ASSERT(inst->opcode() == G4_pseudo_not,
                  "Must be a pseudo-not instruction");
      // for not P1 P0
      // we generate
      // (P0) sel V1 1 0
      // xor.P1 null V1 1
      // so that the upper bits would stay zero
      logicSrc1 = builder.createImm(1, Type_UW);
    }

    auto nullDst = builder.createNullDst(Type_UW);
    auto newCondMod =
        builder.createCondMod(Mod_nz, inst->getDst()->getBase()->asRegVar(), 0);
    G4_opcode newOpcode = G4_illegal;
    switch (inst->opcode()) {
    case G4_pseudo_and:
      newOpcode = G4_and;
      break;
    case G4_pseudo_or:
      newOpcode = G4_or;
      break;
    case G4_pseudo_xor:
      newOpcode = G4_xor;
      break;
    case G4_pseudo_not:
      // see comment above
      newOpcode = G4_xor;
      break;
    default:
      vISA_ASSERT_UNREACHABLE(
          "unexpected opcode for pseudo-logic instructions");
    }

    G4_INST *newLogicOp = builder.createInternalInst(
        NULL, newOpcode, newCondMod, g4::NOSAT, tmpSize, nullDst, logicSrc0,
        logicSrc1,
        inst->getOption() // keep the original instruction emask
    );

    // Fix def-use
    if (Sel0 != nullptr) {
      Sel0->addDefUse(newLogicOp, Gen4_Operand_Number::Opnd_src0);
    }
    if (Sel1 != nullptr) {
      Sel1->addDefUse(newLogicOp, Gen4_Operand_Number::Opnd_src1);
    }
    inst->transferUse(newLogicOp);
    bb->insertBefore(newIter, newLogicOp);
    bb->erase(newIter);
  }

  // iter either points to the start or the first expanded instruction.  Caller
  // will advance it to the previous instruction
  if (isFirstInst) {
    iter = bb->begin();
  } else {
    ++iter;
  }
}

// mov(1) P0 Imm(NoMask)
// (P0) mov(esize) r[A0, 0] src0 src1
// == >
// smov(esize) r[A0, 0] src0 src1 Imm
//
// esize is either 8 or 16
bool Optimizer::createSmov(G4_BB *bb, G4_INST *flagMove, G4_INST *next_inst) {
  if ((next_inst->getExecSize() != g4::SIMD8 &&
       next_inst->getExecSize() != g4::SIMD16) ||
      next_inst->getPredicate() == NULL || next_inst->getCondMod() != NULL ||
      next_inst->getSaturate() == true ||
      next_inst->getDst()->getRegAccess() == Direct ||
      next_inst->getDst()->getTypeSize() == 1 ||
      next_inst->getSrc(0)->getTypeSize() == 1 ||
      (builder.getPlatform() < GENX_SKL && builder.getPlatform() != GENX_BDW) ||
      next_inst->getDst()->getTypeSize() <
          next_inst->getSrc(0)->getTypeSize()) {
    return false;
  }

  if (next_inst->getSrc(0)->isSrcRegRegion() &&
      next_inst->getSrc(0)->asSrcRegRegion()->getModifier() != Mod_src_undef) {
    return false;
  }

  if (flagMove->use_size() != 1 || flagMove->use_front().first != next_inst) {
    return false;
  }

  G4_CmpRelation rel =
      flagMove->getDst()->compareOperand(next_inst->getPredicate(), builder);
  if (rel != Rel_eq && !(rel == Rel_gt && next_inst->getMaskOffset() == 0)) {
    return false;
  }

  if (kernel.getKernelType() == VISA_3D || !bb->isAllLaneActive()) {
    if (!flagMove->isWriteEnableInst()) {
      return false;
    }
  }

  next_inst->setOpcode(G4_smov);
  next_inst->setSrc(flagMove->getSrc(0), 1);
  next_inst->setPredicate(nullptr);

  flagMove->removeUseOfInst();
  return true;
}

// Returns true if *iter has an use that is a cmp and we can fold that cmp
// into *iter as a conditional modifier. The cmp instruction is deleted as part
// of folding. Note that iter may be modified to point to the next inst if we
// decide to sync *iter to where the cmp was to work around dependencies
bool Optimizer::foldCmpToCondMod(G4_BB *bb, INST_LIST_ITER &iter) {
  // find a cmp that uses inst dst
  G4_INST *inst = *iter;
  G4_INST *cmpInst = nullptr;
  bool canFold = false;

  if (inst->getCondMod()) {
    return false;
  }

  for (auto UI = inst->use_begin(), UE = inst->use_end(); UI != UE; ++UI) {
    cmpInst = (*UI).first;

    // cmp instruction must be of the form
    // cmp [<op> P0] null src 0
    // where src is singly defined by inst
    if (cmpInst->opcode() == G4_cmp &&
        cmpInst->getExecSize() == inst->getExecSize() &&
        cmpInst->hasNULLDst() && cmpInst->getSrc(0)->isSrcRegRegion() &&
        cmpInst->getSrc(0)->asSrcRegRegion()->getModifier() == Mod_src_undef &&
        cmpInst->def_size() == 1 && !cmpInst->getPredicate() &&
        cmpInst->getSrc(1)->isImm() && cmpInst->getSrc(1)->asImm()->isZero()) {
      canFold = true;
      break;
    }
  }

  if (!canFold) {
    return false;
  }

  // floating point cmp may flush denorms to zero, but mov may not.
  //
  // mov(1|M0) (lt)f0.0 r6.2<1>:f r123.2<0;1,0>:f
  // may not be the same as
  // mov(1|M0)          r6.2<1>:f r123.2<0;1,0>:f
  // cmp(1|M0) (lt)f0.0 null<1>:f 6.2<0;1,0>:f 0x0:f
  // for denorm inputs.
  if (inst->opcode() == G4_mov &&
      IS_TYPE_FLOAT_ALL(cmpInst->getSrc(0)->getType())) {
    return false;
  }

  // If dst is B and exectype is Q, dst is misaligned. This misaligned inst
  // will be split in HWComformity. If a condMod is present, it would be
  // harder to split and may result in wrong code. Here, simply disable folding.
  //
  // For example,
  //    (W) mov (1)     conv_i(0,0)<1>:b   V0040(0,0)<0;1,0>:q
  //    (W) cmp(1)  (eq)P01.0   null<1>:w  conv_i(0, 0)<0;1,0>:b  0:w
  //
  int extypesize = 0;
  (void)inst->getOpExecType(extypesize);
  if (inst->getDst()->getTypeSize() == 1 && extypesize == 8) {
    return false;
  }

  auto cmpIter = std::find(iter, bb->end(), cmpInst);

  // check if cmp instruction is close enough
  constexpr int DISTANCE = 60;
  if (std::distance(iter, cmpIter) > DISTANCE) {
    return false;
  }

  auto isSupportedCondMod = [](G4_CondModifier mod) {
    return mod == Mod_g || mod == Mod_ge || mod == Mod_l || mod == Mod_le ||
           mod == Mod_e || mod == Mod_ne;
  };
  G4_CondModifier mod = cmpInst->getCondMod()->getMod();
  if (!isSupportedCondMod(mod)) {
    return false;
  }

  // and/or/xor/not opcodes support only the conditional modifiers
  // .e/.z or .ne.nz
  auto opcode = inst->opcode();
  bool isLogicOp = (opcode == G4_xor || opcode == G4_or || opcode == G4_and ||
                    opcode == G4_not);
  bool isSupportedCondModForLogicOp =
      (mod == Mod_e || mod == Mod_z || mod == Mod_ne || mod == Mod_nz);
  if (isLogicOp && !isSupportedCondModForLogicOp)
    return false;

  if (kernel.getKernelType() == VISA_3D || !bb->isAllLaneActive()) {
    // Make sure masks of both instructions are same
    if (inst->getMaskOption() != cmpInst->getMaskOption()) {
      return false;
    }
  }

  auto getUnsignedTy = [](G4_Type Ty) {
    switch (Ty) {
    case Type_D:
      return Type_UD;
    case Type_W:
      return Type_UW;
    case Type_B:
      return Type_UB;
    case Type_Q:
      return Type_UQ;
    case Type_V:
      return Type_UV;
    default:
      break;
    }
    return Ty;
  };

  G4_Type T1 = inst->getDst()->getType();
  G4_Type T2 = cmpInst->getSrc(0)->getType();
  if (getUnsignedTy(T1) != getUnsignedTy(T2)) {
    return false;
  }
  if (!isSupportedCondModForLogicOp && T1 != T2) {
    // If dst signedness of inst is not same as cmp src0, then only z/nz
    // conditions can be evaluated correctly.
    //
    // If inst is a type-conversion mov then it's incorrect to use cmp src0
    // type as mov dst type.
    //
    // mov A:d   B:f    // f->d mov
    // cmp.gt P1 A:ud   0x0
    //
    // When folding cmp in the mov, we must preserve mov's dst type :d.
    // Otherwise type-conversion semantics change which can lead to wrong
    // result if f->d yields negative result.
    //
    // But if cmp used cmp.z/nz then folding is legal.
    bool isDstSigned = IS_SIGNED_INT(T1);
    bool isDstUnsigned = IS_SIGNED_INT(T1);
    bool isSrcSigned = IS_SIGNED_INT(T2);
    bool isSrcUnsigned = IS_SIGNED_INT(T2);
    if (!(isDstSigned && isSrcSigned) && !(isDstUnsigned && isSrcUnsigned))
      return false;
  }

  // Skip if the dst needs saturating but it's used as different sign.
  if (inst->getSaturate() && T1 != T2) {
    return false;
  }

  if (chkBwdOutputHazard(iter, cmpIter)) {
    return false;
  }

  G4_Declare *dstDcl = GetTopDclFromRegRegion(inst->getDst());
  if (dstDcl->getAddressed() && chkBwdWAWdep(inst, cmpIter)) {
    return false;
  }

  auto isSafeToSink = [this](INST_LIST_ITER defIter, INST_LIST_ITER beforeIter,
                             int maxDist) {
    G4_INST *inst = *defIter;
    int dist = 0;
    for (auto it = std::next(defIter); it != beforeIter; ++it) {
      if (dist++ >= maxDist)
        return false;
      if (inst->isWAWdep(*it) || inst->isRAWdep(*it) || inst->isWARdep(*it))
        return false;
      if (!checkLifetime(inst, *it))
        return false;
      if (inst->isAccSrcInst() && builder.hasMacMacl() &&
          (*it)->opcode() == G4_mul && IS_DTYPE((*it)->getSrc(0)->getType()) &&
          IS_DTYPE((*it)->getSrc(1)->getType())) {
        // Do not sink instructions with explicit ACC src over mul
        // instructions as mul can be changed to:
        //   mul (8) acc0.0<1>:d src0:d src1:w
        //   mach (8) dst:d src0:d src1:d
        // see HWConformity::generateMacl()
        return false;
      }
    }
    return true;
  };

  // Merge two instructions
  // If legal, use the cmp location as new insert position.
  bool sinkInst = false;

  if (inst->getDst()->compareOperand(cmpInst->getSrc(0), builder) == Rel_eq) {
    if (inst->use_size() == 1) {
      // see if we can replace dst with null
      if (inst->supportsNullDst() &&
          !fg.globalOpndHT.isOpndGlobal(inst->getDst())) {
        inst->setDest(builder.createDst(builder.phyregpool.getNullReg(), 0, 0,
                                        inst->getDst()->getHorzStride(),
                                        inst->getDst()->getType()));
      }
      // Check if it is safe to sink inst right before cmp inst, which lowers
      // flag pressure in general.
      const int MAX_DISTANCE = 20;
      sinkInst = isSafeToSink(iter, cmpIter, MAX_DISTANCE);
    }
    inst->setCondMod(cmpInst->getCondMod());
    inst->setOptions((inst->getOption() & ~InstOpt_Masks) |
                     (cmpInst->getMaskOption()));
    // The sign of dst should follow its use instead of its
    // def. The later is meaningless from how hardware works.
    auto honorSignedness = [](G4_CondModifier mod) {
      switch (mod) {
      case Mod_g:
      case Mod_ge:
      case Mod_l:
      case Mod_le:
        return true;
      default:
        break;
      }
      return false;
    };
    if (honorSignedness(inst->getCondMod()->getMod()))
      inst->getDst()->setType(builder, T2);

    // update def-use
    // since cmp is deleted, we have to
    // -- transfer cmp's use to inst
    // -- remove cmp from its definition's use list
    cmpInst->transferUse(inst, true);
    cmpInst->removeUseOfInst();
    if (!sinkInst) {
      bb->erase(cmpIter);
    } else {
      // Before and <- ii
      //        cmp <- next_iter
      // After  cmp <- ii
      //        and <- next
      std::iter_swap(iter, cmpIter);
      auto nextii = std::next(iter);
      bb->erase(iter);
      iter = nextii;
    }
    return true;
  }
  return false;
}

// Return true  : folding cmp and sel is performed
//        false : no folding is performed.
bool Optimizer::foldCmpSel(G4_BB *BB, G4_INST *selInst,
                           INST_LIST_ITER &selInst_II) {
  vISA_ASSERT(
      (selInst->opcode() == G4_sel && selInst->getPredicate() &&
       selInst->getCondMod() == NULL),
      "foldCmpSel: Inst should be a sel with predicate and without cmod.");
  G4_Predicate *pred = selInst->getPredicate();

  // global predicates are not eligible since folding the cmp removes the def
  if (fg.globalOpndHT.isOpndGlobal(pred)) {
    return false;
  }

  // Needs to find cmp instruction that defines predicate of sel. The cmp is
  // not necessarily right before the sel. To be able to fold the cmp to the
  // sel, we have to check if the cmp can be moved right before the sel. It not,
  // no folding is performed.
  G4_INST *cmpInst = nullptr;
  for (auto DI = selInst->def_begin(), DE = selInst->def_end(); DI != DE;
       ++DI) {
    if ((*DI).second == Opnd_pred) {
      if (cmpInst) { // only handle single def.
        cmpInst = nullptr;
        break;
      }
      cmpInst = (*DI).first;
      if (cmpInst && cmpInst->opcode() != G4_cmp) {
        cmpInst = nullptr;
        break;
      }
    }
  }
  if (!cmpInst) {
    // G4_cmp is not found, skip optimization.
    return false;
  }

  // Do a fast check first. Note that sel w/ cmod
  // does not allow predication. So, we will give up if cmp has predicate.
  bool isSubExecSize = (selInst->getExecSize() < cmpInst->getExecSize());
  bool isSameExecSize = (selInst->getExecSize() == cmpInst->getExecSize());
  if ((!isSameExecSize && !isSubExecSize) ||
      (cmpInst->getDst() && !cmpInst->hasNULLDst()) ||
      cmpInst->use_size() != 1 || cmpInst->getPredicate() != nullptr) {
    return false;
  }

  // Check if two source operands have the same type and value.
  auto IsEqual = [](G4_Operand *opnd1, G4_Operand *opnd2,
                    const IR_Builder &builder) {
    if (opnd1->isImm() && opnd2->isImm())
      return opnd1->asImm()->isEqualTo(opnd2->asImm());
    if (opnd1->compareOperand(opnd2, builder) != Rel_eq)
      return false;
    // footprint does not imply equality.
    // (1) region difference:  r10.0<1;4,2>:f vs r10.0<8;8,1>
    // (2) source modifier: r10.0<0;1,0>:f vs -r10.0<0;1,0>
    //
    if (opnd1->isSrcRegRegion() && opnd2->isSrcRegRegion())
      return opnd1->asSrcRegRegion()->sameSrcRegRegion(
          *opnd2->asSrcRegRegion());

    // Common cases should be covered.
    return false;
  };

  G4_Operand *sel_src0 = selInst->getSrc(0);
  G4_Operand *sel_src1 = selInst->getSrc(1);
  G4_Operand *cmp_src0 = cmpInst->getSrc(0);
  G4_Operand *cmp_src1 = cmpInst->getSrc(1);

  // Normalize SEL's predicate state to Plus.
  if (G4_PredState::PredState_Minus == pred->getState()) {
    selInst->swapSrc(0, 1);
    selInst->swapDefUse();
    pred->setState(G4_PredState::PredState_Plus);
    std::swap(sel_src0, sel_src1);
  }

  // source operands of SEL and CMP are reversed or not.
  bool reversed = false;
  G4_CondMod *condMod = cmpInst->getCondMod();

  auto canFold = [=, &reversed](const IR_Builder &builder) {
    G4_CmpRelation condRel =
        pred->asPredicate()->compareOperand(condMod, builder);
    if (!(condRel == Rel_eq && isSameExecSize) &&
        !(condRel == Rel_lt && isSubExecSize))
      return false;

    if (!cmpInst->isWriteEnableInst() &&
        cmpInst->getMaskOption() != selInst->getMaskOption())
      return false;

    // P = cmp.gt A, B
    // C = (+P) sel A, B  => C = sel.gt A, B
    //
    // P = cmp.ne A, B
    // C = (+P) sel A, B  => C = sel.ne A, B
    //
    if (IsEqual(sel_src0, cmp_src0, builder) &&
        IsEqual(sel_src1, cmp_src1, builder))
      return true;

    // Sel operands are reversed.
    // P = cmp.gt A, B
    // C = (+P) sel B, A  => C = sel.le B, A
    //
    // P = cmp.ne A, B
    // C = (+P) sel B, A  => C = sel.ne B, A
    //
    if (IsEqual(sel_src0, cmp_src1, builder) &&
        IsEqual(sel_src1, cmp_src0, builder)) {
      reversed = true;
      // In case of float cmp with possible NaN operands, should not swap
      // operands.
      if (builder.getOption(vISA_finiteMathOnly) ||
          !IS_TYPE_FLOAT_ALL(cmp_src0->getType())) {
        return true;
      }
    }
    return false;
  };

  if (!canFold(builder)) {
    return false;
  }

  // check if cmpInst can be legally moved right before selInst;
  // if it cannot, we cannot fold cmp to sel!.
  INST_LIST_ITER cmpInst_II = selInst_II;
  while (cmpInst_II != BB->begin()) {
    cmpInst_II--;
    if (*cmpInst_II == cmpInst) {
      break;
    }
  }

  // No local def (possible?)
  if (cmpInst_II == BB->begin()) {
    return false;
  }

  // If cmpInst has no WAR harzard b/w cmpInst and selInst, cmpInst
  // can be moved right before selInst.
  if (chkFwdOutputHazard(cmpInst_II, selInst_II)) {
    return false;
  }

  G4_CondModifier mod = condMod->getMod();
  if (reversed)
    mod = G4_CondMod::getReverseCondMod(mod);
  G4_CondMod *cmod =
      builder.createCondMod(mod, condMod->getBase(), condMod->getSubRegOff());
  selInst->setCondMod(cmod);
  selInst->setPredicate(nullptr);

  // update def-use
  // since cmp is deleted, we have to
  // -- remove def-use between cmp and sel
  // -- transfer cmp's remaining use to sel
  // -- remove cmp and its definitions' use
  selInst->removeDefUse(Opnd_pred);
  cmpInst->transferUse(selInst, true);
  cmpInst->removeUseOfInst();
  BB->erase(cmpInst_II);
  return true;
}

// try to fold a pseudo not instruction into its use(s)
// return true if successful
bool Optimizer::foldPseudoNot(G4_BB *bb, INST_LIST_ITER &iter) {
  G4_INST *notInst = *iter;
  vISA_ASSERT(notInst->opcode() == G4_pseudo_not, "expect not instruction");
  if (notInst->getPredicate() || notInst->getCondMod()) {
    return false;
  }

  G4_DstRegRegion *dst = notInst->getDst();
  if (fg.globalOpndHT.isOpndGlobal(dst)) {
    return false;
  }
  if (!notInst->getSrc(0)->isSrcRegRegion()) {
    return false;
  }

  // unfortunately, def-use chain is not always properly maintained, so we have
  // to skip opt even if we can't find a use
  bool canFold = notInst->use_size() > 0;
  for (auto uses = notInst->use_begin(), end = notInst->use_end(); uses != end;
       ++uses) {
    auto &&use = *uses;
    G4_INST *useInst = use.first;
    Gen4_Operand_Number opndPos = use.second;
    if (!useInst->isLogic() || !G4_INST::isSrcNum(opndPos) ||
        useInst->getSingleDef(opndPos) == nullptr /* not single def */) {
      canFold = false;
      break;
    }

    // sanity check
    vASSERT(useInst->getSingleDef(opndPos) == notInst);

    // check the case where flag is partially used
    G4_SrcRegRegion *opnd =
        useInst->getSrc(G4_INST::getSrcNum(opndPos))->asSrcRegRegion();
    if (dst->compareOperand(opnd, builder) != Rel_eq) {
      canFold = false;
      break;
    }
  }

  if (canFold) {
    G4_SrcRegRegion *origUse = notInst->getSrc(0)->asSrcRegRegion();
    if (notInst->getMaskOffset() == 16) {
      // Fold upper bits
      vASSERT(notInst->getExecSize() == g4::SIMD16);
      origUse = builder.createSrcWithNewSubRegOff(origUse, 1);
      notInst->setSrc(origUse, 0);
    }
    for (auto uses = notInst->use_begin(), uend = notInst->use_end();
         uses != uend; ++uses) {
      auto use = *uses;
      G4_INST *useInst = use.first;
      Gen4_Operand_Number opndPos = use.second;
      G4_SrcRegRegion *opnd =
          useInst->getSrc(G4_INST::getSrcNum(opndPos))->asSrcRegRegion();
      int numNot = 1 + (origUse->getModifier() == Mod_Not ? 1 : 0) +
                   (opnd->getModifier() == Mod_Not ? 1 : 0);
      G4_SrcModifier newModifier = numNot & 0x1 ? Mod_Not : Mod_src_undef;
      G4_SrcRegRegion *newSrc = builder.createSrcRegRegion(*origUse);
      newSrc->setModifier(newModifier);
      useInst->setSrc(newSrc, G4_INST::getSrcNum(opndPos));
    }

    for (auto defs = notInst->def_begin(); defs != notInst->def_end(); ++defs) {
      auto def = *defs;
      G4_INST *defInst = def.first;
      notInst->copyUsesTo(defInst, false);
    }
    notInst->removeAllDefs();
    notInst->removeAllUses();
    bb->erase(iter);
    return true;
  }
  return false;
}

/***
this function optimizes the following cases:

case 1:
cmp.gt.P0 s0 s1
(P0) sel d s0 s1
==>
sel.gt.P0 d s0 s1

case 2:
and dst src0 src1   -- other OPs are also optimized: or, xor...
cmp.nz.P0 NULL dst 0
(P0) ...
==>
and.nz.P0 dst src0 src1
(P0) ...
add/addc instructions also handled in case 2. Few more cond
modifiers supported to such arithmetic instructions.

case 3:
cmp.l.P0 NULL src0 src1
cmp.l.P1 NULL src2 src3
and P2 P0 P1   -- other OPs are also optimized: or, xor...
==>
cmp.l.P2 NULL src0 src1
(P2)cmp.l.P2 NULL src2 src3

case 4:
mov (1) P0 Imm  (NoMask)
(P0) mov (8) r[A0, 0] src0 src1
==>
smov (8) r[A0, 0] src0 src1 Imm

case 5:
pseudo_not (1) P2 P1
and (1) P4 P3 P2
==>
and (1) P4 P3 ~P1
*/

void Optimizer::optimizeLogicOperation() {
  G4_Operand *dst = NULL;
  bool resetLocalIds = false;
  bool doLogicOpt = builder.getOption(vISA_LocalFlagOpt);

  if (!doLogicOpt) {
    // we still need to expand the pseudo logic ops
    for (auto bb : fg) {
      for (auto I = bb->begin(), E = bb->end(); I != E; ++I) {
        auto inst = *I;
        if (inst->isPseudoLogic()) {
          expandPseudoLogic(builder, bb, I);
        }
      }
    }
    return;
  }

  for (G4_BB *bb : fg) {
    INST_LIST_ITER ii;
    if ((bb->begin() == bb->end())) {
      continue;
    }
    resetLocalIds = false;
    ii = bb->end();
    do {
      ii--;
      G4_INST *inst = *ii;
      G4_opcode op = inst->opcode();
      dst = inst->getDst();
      bool nullDst = inst->hasNULLDst();
      G4_Declare *dcl = nullptr;
      if (dst) {
        dcl = dst->getTopDcl();
      }

      if ((op != G4_sel && op != G4_cmp && !inst->canSupportCondMod() &&
           !inst->isPseudoLogic()) ||
          !dst || nullDst || (dcl && dcl->isOutput())) {
        continue;
      }

      INST_LIST_ITER next_iter = std::next(ii);

      if (!resetLocalIds) {
        bb->resetLocalIds();
        resetLocalIds = true;
      }

      // case 5
      if (inst->opcode() == G4_pseudo_not) {
        bool folded = foldPseudoNot(bb, ii);
        if (folded) {
          ii = next_iter;
          continue;
        }
      }

      // case 1
      if (ii != bb->begin() && op == G4_sel && inst->getPredicate() &&
          !inst->getCondMod()) {
        foldCmpSel(bb, inst, ii);
      } else if (builder.hasSmov() && inst->opcode() == G4_mov &&
                 inst->getPredicate() == NULL && inst->getCondMod() == NULL &&
                 inst->getExecSize() == g4::SIMD1 && inst->getSrc(0)->isImm() &&
                 inst->getDst()->isFlag() && next_iter != bb->end() &&
                 (*next_iter)->opcode() == G4_mov) {
        // case 4
        G4_INST *next_inst = *next_iter;
        if (createSmov(bb, inst, next_inst)) {
          bb->erase(ii);
          ii = next_iter;
        }
        continue;
      } else if (!inst->getPredicate() && inst->canSupportCondMod()) {
        // FIXME: why this condition?
        if (op == G4_pseudo_mad && inst->getExecSize() == g4::SIMD1) {
          continue;
        }

        // case 2
        foldCmpToCondMod(bb, ii);
      } else if (inst->getPredicate() == NULL && inst->isPseudoLogic()) {
        bool merged = false;

        if (op == G4_pseudo_and || op == G4_pseudo_or) {
          merged = foldPseudoAndOr(bb, ii);
        }

        // translate the pseudo op
        if (!merged) {
          expandPseudoLogic(builder, bb, ii);
        }
      }
    } while (ii != bb->begin());
  }
}

// see if we can fold a pseudo and/or instruction with previous cmp
// returns true if successful, and ii (inst-list-iter) is also updated
// to the next inst
bool Optimizer::foldPseudoAndOr(G4_BB *bb, INST_LIST_ITER &ii) {
  // case 3

  // optimization should apply even when the dst of the pseudo-and/pseudo-or is
  // global, since we are just hoisting it up, and WAR/WAW checks should be
  // performed as we search for the src0 and src1 inst. Also need to check if
  // the mask option of the pseudo-and/pseudo-or matches with the options of
  // the defining instructions when dst is global.

  G4_INST *inst = *ii;
  // look for def of srcs
  G4_Operand *src0 = inst->getSrc(0);
  G4_Operand *src1 = inst->getSrc(1);

  /*
  Old logic would scan from inst (and/or) up until it encountered
  def instructions that did full write.
  It would scan from def instruction down to inst looking for WAW, WAR
  conflicts between def instruction and intermediate instructions.
  Basically insuring that there were no partial writes in to flag registers
  use by the inst.

  The new code uses defInstList directly, and aborts if there are more then are
  two definitions. Which means there is more then one instruction writing to
  source. Disadvantage of that is that it is less precise. For example if we
  are folding in to closest definition then before it was OK, but now will be
  disallowed.
  */
  int32_t maxSrc1 = 0;
  int32_t maxSrc2 = 0;
  G4_INST *defInstructions[2] = {nullptr, nullptr};
  G4_INST *src0DefInst = nullptr;
  G4_INST *src1DefInst = nullptr;

  // Picks the latest two instructions to compare.
  // local IDs are reset at the beginning of this function.
  if (inst->def_size() < 2) {
    return false;
  }
  // trying to find latest instructions that define src0 and src1
  for (auto I = inst->def_begin(), E = inst->def_end(); I != E; ++I) {
    G4_INST *srcInst = I->first;
    if ((I->second == Opnd_src0) && (srcInst->getLocalId() >= maxSrc1)) {
      maxSrc1 = srcInst->getLocalId();
      defInstructions[0] = srcInst;
      src0DefInst = srcInst;
    } else if ((I->second == Opnd_src1) && (srcInst->getLocalId() >= maxSrc2)) {
      maxSrc2 = srcInst->getLocalId();
      defInstructions[1] = srcInst;
      src1DefInst = srcInst;
    }
  }

  // Making sure that dst of pseudo instruction is not used or defined between
  // pseudo instruction and the first definition of the source
  if (defInstructions[0] && defInstructions[1]) {
    // make defInst[0] the closer def to the pseudo-and/or
    if (maxSrc2 > maxSrc1) {
      std::swap(defInstructions[0], defInstructions[1]);
      std::swap(maxSrc1, maxSrc2);
    }
    // Doing backward scan until earliest src to make sure dst of and/or is not
    // being written to or being read
    /*
    handling case like in spmv_csr
    cmp.lt (M1, 1) P15 V40(0,0)<0;1,0> 0x10:w /// $191
    cmp.lt (M1, 1) P16 V110(0,0)<0;1,0> V34(0,0)<0;1,0> /// $192
    and    (M1, 1) P16 P16 P15 /// $193
    */
    if (chkBwdOutputHazard(defInstructions[1], ii, defInstructions[0])) {
      return false;
    }
  } else {
    return false;
  }

  // check if the defInst can be folded into the pseudo and/or for a given
  // source folding is legal if
  // -- src is the only use of defInst
  // -- def completely defines the src
  // -- def inst does not have predicate
  // -- the defInst closer to the pseudo inst is a cmp without pred
  // -- def inst is not global operand
  // the last condition can be relaxed if defInst is the same as the inst's dst,
  // as they will be the same flag
  if (!(defInstructions[0]->opcode() == G4_cmp &&
        defInstructions[0]->getPredicate() == nullptr)) {
    return false;
  }

  auto checkSource = [this, inst](G4_INST *defInst, G4_Operand *src) {
    if (defInst == nullptr) {
      return false;
    }

    if (defInst->use_size() > 1) {
      return false;
    }
    G4_Operand *curr_dst = defInst->getCondMod()
                               ? defInst->getCondMod()
                               : (G4_Operand *)defInst->getDst();

    G4_CmpRelation rel = curr_dst->compareOperand(src, builder);
    if (rel != Rel_eq ||
        (defInst->getPredicate() && defInst->opcode() != G4_sel)) {
      return false;
    }

    if (fg.globalOpndHT.isOpndGlobal(curr_dst)) {
      G4_DstRegRegion *dst = inst->getDst();
      if (dst->compareOperand(curr_dst, builder) != Rel_eq) {
        return false;
      }
    }

    return true;
  };

  if (!checkSource(src0DefInst, src0) || !checkSource(src1DefInst, src1)) {
    return false;
  }

  // Check if mask options are mismatched between the pseudo-and/pseudo-or and
  // its defining instructions.
  if ((inst->getMaskOption() != src0DefInst->getMaskOption() ||
       inst->getMaskOption() != src1DefInst->getMaskOption()) &&
      fg.globalOpndHT.isOpndGlobal(inst->getDst()) &&
      !fg.builder->alwaysAllowGlobalFlagOpt())
    return false;

  // do the case 3 optimization

  G4_PredState ps =
      (inst->opcode() == G4_pseudo_or) ? PredState_Minus : PredState_Plus;
  G4_INST *first_inst = defInstructions[1];
  G4_INST *second_inst = defInstructions[0];

  // unify subregister according to logic op dst
  G4_VarBase *curr_base = inst->getDst()->getBase()->asRegVar();
  unsigned short curr_subreg = 0;

  G4_Operand *first_def, *second_def;
  G4_VarBase *first_def_base, *second_def_base;
  int first_def_subreg, second_def_subreg;

  // change condmod and predicate of second inst

  if (first_inst->getCondMod()) {
    first_def = first_inst->getCondMod();
    first_def_base = first_def->asCondMod()->getBase();
    first_def_subreg = first_def->asCondMod()->getSubRegOff();
  } else {
    first_def = first_inst->getDst();
    first_def_base = first_def->asDstRegRegion()->getBase()->asRegVar();
    first_def_subreg = 0;
  }

  if (second_inst->getCondMod()) {
    second_def = second_inst->getCondMod();
    second_def_base = second_def->asCondMod()->getBase();
    second_def_subreg = second_def->asCondMod()->getSubRegOff();
  } else {
    second_def = second_inst->getDst();
    second_def_base = second_def->asDstRegRegion()->getBase()->asRegVar();
    second_def_subreg = 0;
  }

  bool change_flag = false;
  if (second_inst->getCondMod() &&
      (second_def_base != curr_base || second_def_subreg != curr_subreg)) {
    change_flag = true;
    G4_CondMod *new_condMod = builder.createCondMod(
        second_inst->getCondMod()->getMod(), curr_base, curr_subreg);

    second_inst->setCondMod(new_condMod);
  }
  // create a predicate

  G4_Predicate *new_pred = builder.createPredicate(ps, curr_base, curr_subreg);

  second_inst->setPredicate(new_pred);

  if (change_flag) {
    for (USE_EDGE_LIST_ITER iter = second_inst->use_begin();
         iter != second_inst->use_end(); ++iter) {
      G4_INST *curr_inst = (*iter).first;
      if (curr_inst == inst) {
        continue;
      }
      if (curr_inst->getPredicate() &&
          (curr_inst->getPredicate()->getBase() != curr_base ||
           curr_inst->getPredicate()->getSubRegOff() != curr_subreg) &&
          curr_inst->getPredicate()->compareOperand(second_def, builder) ==
              Rel_eq) {
        curr_inst->setPredicate(builder.duplicateOperand(new_pred));
      }

      for (int k = 0; k < curr_inst->getNumSrc(); k++) {
        G4_Operand *curr_src = curr_inst->getSrc(k);
        if (curr_src->isSrcRegRegion() &&
            !(curr_inst->isMath() && k == 1 && curr_src->isNullReg())) {
          if (curr_src->asSrcRegRegion()->compareOperand(second_def, builder) ==
              Rel_eq) {
            G4_SrcRegRegion *new_src_opnd = builder.createSrcRegRegion(
                curr_src->asSrcRegRegion()->getModifier(),
                curr_src->asSrcRegRegion()->getRegAccess(),
                inst->getDst()->getBase(), 0, 0,
                curr_src->asSrcRegRegion()->getRegion(),
                curr_src->asSrcRegRegion()->getType());

            curr_inst->setSrc(new_src_opnd, k);
          }
        }
      }
    }
  }

  if (first_def_base != curr_base || first_def_subreg != curr_subreg) {
    if (first_inst->getCondMod() && first_def->isCondMod()) {
      G4_CondMod *new_condMod = builder.createCondMod(
          first_inst->getCondMod()->getMod(), curr_base, curr_subreg);
      first_inst->setCondMod(new_condMod);
    } else {
      first_inst->setDest(builder.duplicateOperand(inst->getDst()));
    }
    for (USE_EDGE_LIST_ITER iter = first_inst->use_begin();
         iter != first_inst->use_end(); ++iter) {
      G4_INST *curr_inst = (*iter).first;
      if (curr_inst == inst) {
        continue;
      }
      if (curr_inst->getPredicate() &&
          (curr_inst->getPredicate()->getBase() != curr_base ||
           curr_inst->getPredicate()->getSubRegOff() != curr_subreg) &&
          curr_inst->getPredicate()->compareOperand(first_def, builder) ==
              Rel_eq) {
        curr_inst->setPredicate(builder.duplicateOperand(new_pred));
      }

      for (int k = 0; k < curr_inst->getNumSrc(); k++) {
        G4_Operand *curr_src = curr_inst->getSrc(k);
        if (curr_src->isSrcRegRegion() &&
            !(curr_inst->isMath() && k == 1 && curr_src->isNullReg())) {
          if (curr_src->asSrcRegRegion()->compareOperand(first_def, builder) ==
              Rel_eq) {
            G4_SrcRegRegion *new_src_opnd = builder.createSrcRegRegion(
                curr_src->asSrcRegRegion()->getModifier(),
                curr_src->asSrcRegRegion()->getRegAccess(), curr_base, 0,
                curr_subreg, curr_src->asSrcRegRegion()->getRegion(),
                curr_src->asSrcRegRegion()->getType());
            curr_inst->setSrc(new_src_opnd, k);
          }
        }
      }
    }
  }

  // update def-use
  // since inst (the pseudo-and/or) is deleted and the same flag is used for
  // first and second inst, we have to
  // -- transfer inst's use to the second_inst
  // -- add def-use between first_inst and second_inst
  // -- remove inst from first_inst and second_inst's use
  inst->transferUse(second_inst, true);
  first_inst->addDefUse(second_inst, Opnd_pred);
  inst->removeUseOfInst();

  INST_LIST_ITER new_iter = ii;
  ++ii;
  bb->erase(new_iter);
  return true;
}

/***  The beginning of message header optimization  ***/

/*
 * reuse the previous header which can save the redundant definitions.
 */
void MSGTable::reusePreviousHeader(G4_INST *dest, G4_INST *source,
                                   G4_INST *mDot2, IR_Builder &builder) {
  if (dest == NULL)
    return;
  if (source != NULL) {
    dest->setDest(builder.duplicateOperand(source->getDst()));
  } else {
    short subRegOff = dest->getDst()->getSubRegOff();
    auto newDst = builder.createDstWithNewSubRegOff(mDot2->getDst(), subRegOff);
    dest->setDest(newDst);
  }
}

/*
 * insert a mov, from the previous header to current header
 * this is only required for the instruction,
 * whose payload size >1 so that we can't directly reuse
 * the previous header. keep a copy from the previous header,
 * so that we only need to update the fields that need to be changed.
 */
void MSGTable::insertHeaderMovInst(G4_INST *source_send, IR_Builder &builder,
                                   G4_BB *bb) {
  INST_LIST_ITER pos;

  switch (first) {
  case HEADER_FULL_REGISTER:
    pos = m_it;
    break;
  case HEADER_X:
    pos = mDot0_it;
    break;
  case HEADER_Y:
    pos = mDot1_it;
    break;
  case HEADER_SIZE:
    pos = mDot2_it;
    break;
  default:
    vISA_ASSERT_UNREACHABLE(
        "did not catch the first def instruction correctly");
    return;
  }

  // mov(8) m_new<1>, m_old<8;8:1>  {Align1}
  G4_Declare *srcDcl =
      source_send->getSrc(0)->getBase()->asRegVar()->getDeclare();
  G4_SrcRegRegion *newSrcOpnd =
      builder.createSrcRegRegion(srcDcl, builder.getRegionStride1());

  G4_INST *mov =
      builder.createMov(m->getExecSize(), builder.duplicateOperand(m->getDst()),
                        newSrcOpnd, m->getOption(), false);
  bb->insertBefore(pos, mov);

  // maintain def-use.
  //
  // (1) Uses. m is ready to be deleted.
  m->transferUse(mov);

  // (2) Defs
  // The defs should be from definitions for source->send's src(0).
  //
  // mov (8) V244(1,0)<1>:ud V88(0,0)<8;8,1>:ud {Align1, NoMask}
  // mov (8) V244(0,0)<1>:ud r0.0<8;8,1>:ud {Align1, NoMask}
  // mov (1) V244(0,2)<1>:ud 0x1f:ud {Align1, NoMask}
  // mov (1) V244(0,0)<1>:ud 0:uw {Align1, NoMask}
  // mov (1) V244(0,1)<1>:ud 0:uw {Align1, NoMask}
  // add (1) a0.0<1>:ud r1.1<0;1,0>:ud 0x40a8000:ud {Align1, NoMask}
  // source_send:
  // send (8) null<1>:ud V244(0,0)<8;8,1>:ud a0.0<0;1,0>:ud {Align1, NoMask}
  // mov (8) V89(0,0)<1>:d V34_histogram(1,0)<8;8,1>:d {Align1, Q1}
  // mov (8) V246(1,0)<1>:ud V89(0,0)<8;8,1>:ud {Align1, NoMask}
  // mov (8) V246(0,0)<1>:ud V244(0,0)<8;8,1>:ud {Align1, NoMask} <-- mov
  // mov (8) V246(0,0)<1>:ud r0.0<8;8,1>:ud {Align1, NoMask}      <-- m
  //
  // There are more than one defs here.
  //
  // Note that
  //
  // mov (8) V244(1,0)<1>:ud V88(0,0)<8;8,1>:ud {Align1, NoMask}
  //
  // is a definition of send but not for mov. We enable checked
  // while copying defs.
  source_send->copyDef(mov, Opnd_src0, Opnd_src0, /*checked*/ true);
}

/*
 * compare the two instructions
 * they must have:
 * the same instruction opcode, predicate, condmodifier, optionString
 * the same dst, and number of src args
 */
bool Optimizer::isHeaderOptCandidate(G4_INST *dst, G4_INST *src) {
  if (!dst || !src) {
    return true;
  }

  // Compare instructions
  if (dst->opcode() != src->opcode() || dst->getOption() != src->getOption() ||
      dst->getExecSize() != src->getExecSize() ||
      dst->getPredicate() != src->getPredicate() ||
      dst->getCondMod() != src->getCondMod()) {
    return false;
  }

  if (dst->getNumSrc() != src->getNumSrc()) {
    return false;
  }

  // Compare destination args
  G4_Operand *dst_dest = dst->getDst();
  G4_Operand *src_dest = src->getDst();
  if (!dst_dest || !src_dest) {
    return false;
  }

  return true;
}

/*
 * compare the two instructions to see if they are redundent
 * they must have:
 * the same src args
 * the same def
 */
bool Optimizer::isHeaderOptReuse(G4_INST *dst, G4_INST *src) {
  if (!dst && !src) {
    return true;
  }
  if (!dst || !src) {
    return false;
  }

  for (unsigned int i = 0, numSrc = dst->getNumSrc(); i < numSrc; i++) {
    G4_Operand *opnd = dst->getSrc(i);
    if (opnd && opnd->compareOperand(src->getSrc(i), builder) != Rel_eq) {
      return false;
    }
  }

  // The same number of def instructions
  if (dst->def_size() != src->def_size()) {
    return false;
  }

  if (dst->def_size() == 0) {
    return true; // both have no def at all
  }

  for (auto ii = dst->def_begin(); ii != dst->def_end(); ii++) {
    bool sameDef = false;
    for (auto jj = src->def_begin(); jj != src->def_end(); jj++) {
      if ((*ii).first == (*jj).first && (*ii).second == (*jj).second) {
        sameDef = true;
        break; // break the inner jj loop
      }
    }
    if (sameDef == false) {
      return false;
    }
  }
  return true;
}

bool Optimizer::headerOptValidityCheck(MSGTable *dest, MSGTable *source) {
  if (!isHeaderOptCandidate(dest->a0Dot0, source->a0Dot0) ||
      !isHeaderOptCandidate(dest->mDot0, source->mDot0) ||
      !isHeaderOptCandidate(dest->m, source->m) ||
      !isHeaderOptCandidate(dest->mDot1, source->mDot1) ||
      !isHeaderOptCandidate(dest->mDot2, source->mDot2)) {
    return false;
  }

  if (dest->m) {
    if (!(dest->m->hasOneUse() && dest->m->use_front().first == dest->send)) {
      return false;
    }
  }
  if (dest->mDot0) {
    if (!(dest->mDot0->hasOneUse() &&
          dest->mDot0->use_front().first == dest->send)) {
      return false;
    }
  }
  if (dest->mDot1) {
    if (!(dest->mDot1->hasOneUse() &&
          dest->mDot1->use_front().first == dest->send)) {
      return false;
    }
  }
  if (dest->mDot2) {
    if (!(dest->mDot2->hasOneUse() &&
          dest->mDot2->use_front().first == dest->send)) {
      return false;
    }
  }

  if (dest->send && dest->send->getSrc(0) &&
      dest->send->getSrc(0)->getTopDcl() && source->send &&
      source->send->getSrc(0) && source->send->getSrc(0)->getTopDcl()) {
    unsigned short dstSize, sourceSize;
    dstSize = dest->send->getSrc(0)->getTopDcl()->getTotalElems() *
              dest->send->getSrc(0)->getTopDcl()->getElemSize();
    sourceSize = source->send->getSrc(0)->getTopDcl()->getTotalElems() *
                 source->send->getSrc(0)->getTopDcl()->getElemSize();
    if (dstSize != sourceSize) {
      return false;
    }
  }

  return true;
}

// a class to store all presently valid values, each value is an instruction
// if the number of values exceeds the max allowed, the oldest is removed
class InstValues {
  const int maxNumVal;
  std::list<G4_INST *> values;

public:
  InstValues(int maxCount) : maxNumVal(maxCount) {}

  void addValue(G4_INST *inst) {
    if (values.size() == maxNumVal) {
      values.pop_front();
    }
    values.push_back(inst);
  }

  // delete all values that may be invalid after inst
  void deleteValue(G4_INST *inst) {
    if (inst->isOptBarrier()) {
      values.clear();
      return;
    }

    auto hasIndirectGather = [](G4_INST *inst) {
      for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
        auto src = inst->getSrc(i);
        if (src && src->isSrcRegRegion() &&
            src->asSrcRegRegion()->isIndirect() &&
            src->asSrcRegRegion()->getRegion()->isRegionWH()) {
          return true;
        }
      }
      return false;
    };

    if (hasIndirectGather(inst)) {
      // optimization is likely unprofitable due to high address register
      // pressure in this case. more importantly, it may actually cause RA to
      // fail since we don't spill physical a0.0
      values.clear();
      return;
    }

    G4_DstRegRegion *dst = inst->getDst();
    auto interferes = [dst](G4_INST *valInst) {
      const IR_Builder &builder = valInst->getBuilder();
      G4_DstRegRegion *valDst = valInst->getDst();
      if (dst->compareOperand(valDst, builder) != Rel_disjoint) {
        return true;
      }
      for (int i = 0, numSrc = valInst->getNumSrc(); i < numSrc; ++i) {
        G4_Operand *src = valInst->getSrc(i);
        if (src != nullptr &&
            dst->compareOperand(src, builder) != Rel_disjoint) {
          return true;
        }
      }
      return false;
    };
    if (dst != nullptr) {
      values.remove_if(interferes);
    }
  }

  G4_INST *findValue(G4_INST *inst) {
    for (auto valInst : values) {
      if (inst->opcode() != valInst->opcode() ||
          inst->getExecSize() != valInst->getExecSize()) {
        continue;
      }
      // skip flags for now
      if ((inst->getPredicate() || valInst->getPredicate()) ||
          (inst->getCondMod() || valInst->getCondMod())) {
        continue;
      }
      // emask checks
      if (inst->getMaskOffset() != valInst->getMaskOffset() ||
          inst->isWriteEnableInst() ^ valInst->isWriteEnableInst()) {
        continue;
      }
      // all source should be isomorphic (same type/shape)
      bool srcMatch = true;
      for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
        G4_Operand *src = inst->getSrc(i);
        G4_Operand *valSrc = valInst->getSrc(i);
        if (src == nullptr || valSrc == nullptr ||
            src->compareOperand(valSrc, inst->getBuilder()) != Rel_eq) {
          srcMatch = false;
          break;
        }
      }
      if (srcMatch) {
        return valInst;
      }
    }
    return nullptr;
  }

  void clear() { values.clear(); }
};

G4_Operand *Optimizer::updateSendsHeaderReuse(
    std::vector<std::vector<G4_INST *>> &instLookUpTable,
    std::vector<G4_INST *> &iVector, INST_LIST_ITER endIter) {
  int bSize = (int)iVector.size();
  for (auto &Cache : instLookUpTable) {
    if (Cache.size() == bSize) {
      bool match[8] = {false};
      bool anyMatch = false;
      for (int index = 0; index < bSize; ++index) {
        G4_INST *cInst = Cache[index];
        G4_INST *iInst = iVector[index];

        // opcode check
        if (cInst->opcode() != iInst->opcode() ||
            cInst->getExecSize() != iInst->getExecSize()) {
          continue;
        }
        // flag check
        if (cInst->getPredicate() != iInst->getPredicate() ||
            cInst->getCondMod() != iInst->getCondMod()) {
          continue;
        }
        // emask check
        if (cInst->getMaskOffset() != iInst->getMaskOffset() ||
            cInst->isWriteEnableInst() ^ iInst->isWriteEnableInst()) {
          continue;
        }
        // dst check
        G4_DstRegRegion *cDstRgn = cInst->getDst();
        G4_DstRegRegion *iDstRgn = iInst->getDst();
        if (cDstRgn->getRegOff() != iDstRgn->getRegOff() ||
            cDstRgn->getSubRegOff() != iDstRgn->getSubRegOff() ||
            cDstRgn->getHorzStride() != iDstRgn->getHorzStride() ||
            cDstRgn->getRegAccess() != iDstRgn->getRegAccess() ||
            cDstRgn->getType() != iDstRgn->getType()) {
          continue;
        }

        // all source should be isomorphic (same type/shape) and unaltered
        // between declaration and reuse
        bool srcMatch = true;

        for (int iSrc = 0, numSrc = cInst->getNumSrc(); iSrc < numSrc; ++iSrc) {
          G4_Operand *cOpnd = cInst->getSrc(iSrc);
          G4_Operand *iOpnd = iInst->getSrc(iSrc);
          if (cOpnd == nullptr || iOpnd == nullptr ||
              cOpnd->compareOperand(iOpnd, builder) != Rel_eq) {
            srcMatch = false;
            break;
          }
        }

        if (chkBwdWARdep(cInst, endIter))
          srcMatch = false;

        match[index] = srcMatch;
        anyMatch |= srcMatch;
      }

      if (anyMatch) {
        // at least partial match

        for (int index = 0; index < bSize; ++index) {
          G4_INST *cInst = Cache[index];
          G4_INST *iInst = iVector[index];

          // mark it if there is match
          if (match[index]) {
            iInst->markDead();
            continue;
          }

          // create new dst region to replace one in iVector[i]
          G4_DstRegRegion *cDst = cInst->getDst();
          G4_DstRegRegion *iDst = iInst->getDst();
          G4_DstRegRegion *newDstRegion = builder.createDst(
              cDst->getTopDcl()->getRegVar(), iDst->getRegOff(),
              iDst->getSubRegOff(), iDst->getHorzStride(), iDst->getType());
          iInst->setDest(newDstRegion);

          // update the look-up table list
          Cache[index] = iInst;
        }
        return Cache[0]->getDst();
      }
    }
  }
  return nullptr;
}

void Optimizer::cleanupA0Movs() {
  for (auto bb : fg) {
    InstValues values(4);
    for (auto iter = bb->begin(), iterEnd = bb->end(); iter != iterEnd;) {
      G4_INST *inst = *iter;

      auto isDstExtDesc = [](G4_INST *inst) {
        G4_DstRegRegion *dst = inst->getDst();
        if (dst && dst->getTopDcl() && dst->getTopDcl()->isMsgDesc()) {
          // check that its single use is at src3 of split send
          if (inst->use_size() != 1) {
            return false;
          }
          auto use = inst->use_front();
          G4_INST *useInst = use.first;
          if (useInst->isSend()) {
            return true;
          }
        }
        return false;
      };

      if (isDstExtDesc(inst)) {
        G4_INST *valInst = values.findValue(inst);
        if (valInst != nullptr) {
          VISA_DEBUG_VERBOSE({
            std::cout << "can replace \n";
            inst->emit(std::cout);
            std::cout << "\n with \n";
            valInst->emit(std::cout);
            std::cout << "\n";
          });
          for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I) {
            // each use is in the form of A0(0,0)<0;1,0>:ud in a send
            G4_INST *useInst = I->first;
            Gen4_Operand_Number num = I->second;
            vISA_ASSERT(useInst->isSend(), "use inst must be a send");
            G4_SrcRegRegion *newExDesc =
                builder.createSrc(valInst->getDst()->getBase(), 0, 0,
                                  builder.getRegionScalar(), Type_UD);
            useInst->setSrc(newExDesc, useInst->getSrcNum(num));
          }
          (*iter)->removeAllDefs();
          (*iter)->transferUse(valInst);
          iter = bb->erase(iter);
          continue;
        } else {
          VISA_DEBUG_VERBOSE({
            std::cout << "add new value:\n";
            inst->emit(std::cout);
            std::cout << "\n";
          });
          // this is necessary since for msg desc we always the physical a0.0,
          // so a new inst will invalidate the previous one
          values.deleteValue(inst);
          values.addValue(inst);
        }
      } else {
        G4_DstRegRegion *dst = inst->getDst();
        if (dst && dst->isDirectAddress()) {
          // If the address register is used for none extdesc
          values.clear();
        } else {
          values.deleteValue(inst);
        }
      }
      ++iter;
    }
  }
}

//
// Perform value numbering on writes to the extended msg descriptor for bindless
// access of the form op (1) a0.2<1>:ud src0 src1 src2 {NoMask} and remove
// redundant instructions.  This is limited to within BB
//
void Optimizer::cleanupBindless() {
  kernel.fg.resetLocalDataFlowData();
  kernel.fg.localDataFlowAnalysis();

  // Perform send header cleanup for bindless sampler/surface
  for (auto bb : fg) {
    std::vector<std::vector<G4_INST *>> instLookUpTable;
    std::vector<G4_INST *> instVector;
    for (auto iter = bb->begin(), iterEnd = bb->end(); iter != iterEnd;
         ++iter) {
      G4_INST *inst = *iter;
      G4_DstRegRegion *dst = inst->getDst();

      if (dst != nullptr && dst->getTopDcl() != nullptr &&
          dst->getTopDcl()->getCapableOfReuse()) {
        // it is header definition instruction
        instVector.push_back(inst);
      }

      if (inst->isSplitSend()) {
        G4_Operand *header = inst->getSrc(0);
        G4_Operand *exDesc = inst->getSrc(3);

        // When header has multiple uses other than send, be conservative and
        // do not reuse the cached value. It could be introduced by
        // optimizations like LVN.
        if (header->getTopDcl() && header->getTopDcl()->getCapableOfReuse() &&
            exDesc->isSrcRegRegion() && !instVector.empty() &&
            std::all_of(instVector.begin(), instVector.end(), [&](G4_INST *i) {
              return i->hasOneUse() && i->use_front().first == inst;
            })) {

          // check if we can reuse cached values.
          G4_Operand *value =
              updateSendsHeaderReuse(instLookUpTable, instVector, iter);
          if (!value) {
            // no found, cache the header
            instLookUpTable.push_back(instVector);
          } else {
            // update sends header src
            G4_SrcRegRegion *newHeaderRgn = builder.createSrc(
                value->getBase(), 0, 0, builder.getRegionStride1(), Type_UD);
            inst->setSrc(newHeaderRgn, 0);
          }
        }
        // clear header def
        instVector.clear();
      } else if (inst->isSend()) {
        instVector.clear();
      }
    }
    bb->erase(std::remove_if(bb->begin(), bb->end(),
                             [](G4_INST *inst) { return inst->isDead(); }),
              bb->end());
  }

  for (auto bb : fg) {
    InstValues values(4);
    for (auto iter = bb->begin(), iterEnd = bb->end(); iter != iterEnd;) {
      G4_INST *inst = *iter;

      auto isDstExtDesc = [](G4_INST *inst) {
        G4_DstRegRegion *dst = inst->getDst();
        if (dst && dst->getTopDcl() && dst->getTopDcl()->isMsgDesc()) {
          // if a use is something other than a send, do not perform the
          // optimization
          for (auto use = inst->use_begin(); use != inst->use_end(); use++) {
            G4_INST* useInst = use->first;
            if (!useInst->isSend())
              return false;
          }
          return true;
        }
        return false;
      };

      if (isDstExtDesc(inst)) {
        G4_INST *valInst = values.findValue(inst);
        if (valInst != nullptr) {
          VISA_DEBUG_VERBOSE({
            std::cout << "can replace \n";
            inst->emit(std::cout);
            std::cout << "\n with \n";
            valInst->emit(std::cout);
            std::cout << "\n";
          });
          for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I) {
            // each use is in the form of A0(0,0)<0;1,0>:ud in a send
            G4_INST *useInst = I->first;
            Gen4_Operand_Number num = I->second;
            vISA_ASSERT(useInst->isSend(), "use inst must be a send");
            G4_SrcRegRegion *newExDesc =
                builder.createSrc(valInst->getDst()->getBase(), 0, 0,
                                  builder.getRegionScalar(), Type_UD);
            useInst->setSrc(newExDesc, useInst->getSrcNum(num));
          }
          iter = bb->erase(iter);
          continue;
        } else {
          VISA_DEBUG_VERBOSE({
            std::cout << "add new value:\n";
            inst->emit(std::cout);
            std::cout << "\n";
          });
          // this is necessary since for msg desc we always the physical a0.0,
          // so a new inst will invalidate the previous one
          values.deleteValue(inst);
          values.addValue(inst);
        }
      } else {
        G4_DstRegRegion *dst = inst->getDst();
        if (dst && dst->isDirectAddress()) {
          // If the address register is used for none extdesc
          values.clear();
        } else {
          values.deleteValue(inst);
        }
      }
      ++iter;
    }
  }
}

/*
 * compare the two send and their defs
 * determine whether to remove the redundant mov inst
 * or reuse the previous header
 *
 * 1    mov (8) V152(0,0)<1>:ud r0.0<8;8,1>:ud {Align1, NoMask}
 * 2    mov (1) V152(0,2)<1>:ud 0x7000f:ud {Align1, NoMask}
 * 3    mov (1) V152(0,0)<1>:ud 0:uw {Align1, NoMask}
 * 4    mov (1) V152(0,1)<1>:ud 0:uw {Align1, NoMask}
 * 5    add (1) a0.0<1>:ud r1.0<0;1,0>:ud 0x2490000:ud {Align1, NoMask}
 * 6    send (8) V32_in(0,0)<1>:ud V152(0,0)<8;8,1>:ud a0.0<0;1,0>:ud {Align1,
 * NoMask}
 *
 * 7    mov (8) V154(0,0)<1>:ud r0.0<8;8,1>:ud {Align1, NoMask}
 * 8    mov (1) V152(0,2)<1>:ud 0x1f:ud {Align1, NoMask}
 * 9    mov (1) V154(0,0)<1>:ud 0:uw {Align1, NoMask}
 * 10   mov (1) V154(0,1)<1>:ud 0:uw {Align1, NoMask}
 * 11   add (1) a0.0<1>:ud r1.1<0;1,0>:ud 0x2190000:ud {Align1, NoMask}
 * 12   send (8) V33(0,0)<1>:d V152(0,0)<8;8,1>:ud a0.0<0;1,0>:ud {Align1,
 * NoMask}
 *
 * It is rather tricky to maintain def-use chains for this optimization.
 * The send instruction (Line 12) can reuse Inst[1, 3, 4] and need to
 * keep Inst[8, 11]. The defs for send at line 12 is Inst[1, 3, 4, 8, 11]
 * and the defs for send at line 6 is Inst[1, 2, 3, 4, 5].
 *
 * We take the following approach to maintain def-use.
 *
 * - Starting with initial valid defs, [7, 8, 9, 10, 11].
 *
 * - Each to be removed instruction transfers its use to a proper
 *   previous definition.
 *
 * - Each to be kept instruction remains, even there may have changes
 *   in its definition. For example, dst of move instruction at Line 8
 *   is changed from V154 to V152, but no def-use modification should
 *   be made for this instruction.
 *
 * - No def-use modification should be made to the final send, since
 *   all have been properly set.
 */
void Optimizer::optMessageHeaders(MSGTableList &msgList, G4_BB *bb,
                                  DEFA0 &myA0) {
  unsigned char redundancyCount = 0;
  bool isSameX, isSameY, isSameSize;
  bool replaceOldHeader = false;

  uint16_t payLoadSize;

  MSGTable *dest, *source;
  MSGTable_ITER iter = msgList.begin();

  if (iter == msgList.end()) {
    return;
  }
  dest = *iter; // dest is the front
  iter++;
  if (iter == msgList.end()) {
    return;
  }
  source = *iter; // source is the cached one

  if (!headerOptValidityCheck(dest, source)) {
    return;
  }

  if (isHeaderOptReuse(dest->a0Dot0, myA0.pred) && !myA0.isA0Redef) {
    // Transfer uses of dstDot0 to myA0.pred. This removes uses from
    // dest->a0Dot0 and add to myA0.pred. dest->a0Dot0 to be deleted.
    dest->a0Dot0->transferUse(myA0.pred, /*keepExisting*/ true);
    dest->a0Dot0->markDead();
  }

  payLoadSize = dest->send->getMsgDesc()->getSrc0LenRegs();

  isSameX = isHeaderOptReuse(dest->mDot0, source->mDot0) && !source->isXRedef;

  isSameY = isHeaderOptReuse(dest->mDot1, source->mDot1) && !source->isYRedef;

  isSameSize =
      isHeaderOptReuse(dest->mDot2, source->mDot2) && !source->isSizeRedef;

  if (isSameX && dest->mDot0) {
    redundancyCount++;
  }
  if (isSameY && dest->mDot1) {
    redundancyCount++;
  }
  if (isSameSize && dest->mDot2) {
    redundancyCount++;
  }

  if (payLoadSize > 1 && redundancyCount < MESSAGE_HEADER_THRESHOLD) {
    return; // don't delete if redunant insts >=THRESHold
  };

  if (payLoadSize > 1 && !(redundancyCount == 3 &&
                           dest->send->getSrc(0)->compareOperand(
                               source->send->getSrc(0), builder) == Rel_eq)) {
    dest->insertHeaderMovInst(source->send, builder, bb);
    replaceOldHeader = true;
  }

  { // always remove "mov(8) Mx<1>, r0.0<8;8,1>:ud{Align1}"
    dest->m->markDead();
    if (!replaceOldHeader) {
      dest->m->transferUse(source->m, /*keepExisting*/ true);
      dest->m = source->m;
    }
  }

  if (isSameX && dest->mDot0) {
    dest->mDot0->markDead();
    if (!replaceOldHeader) {
      dest->mDot0->transferUse(source->mDot0, /*keepExisting*/ true);
      dest->mDot0 = source->mDot0;
    }
  } else if (payLoadSize == 1 && dest->mDot0) {
    dest->reusePreviousHeader(dest->mDot0, source->mDot0, source->mDot2,
                              builder);
    if (!replaceOldHeader) {
      source->mDot0 = dest->mDot0;
    }
  }

  if (isSameY && dest->mDot1) {
    dest->mDot1->markDead();
    if (!replaceOldHeader) {
      dest->mDot1->transferUse(source->mDot1, /*keepExisting*/ true);
      dest->mDot1 = source->mDot1;
    }
  } else if (payLoadSize == 1 && dest->mDot1) {
    dest->reusePreviousHeader(dest->mDot1, source->mDot1, source->mDot2,
                              builder);
    if (!replaceOldHeader) {
      source->mDot1 = dest->mDot1;
    }
  }

  if (isSameSize && dest->mDot2) {
    dest->mDot2->markDead();
    if (!replaceOldHeader) {
      dest->mDot2->transferUse(source->mDot2, /*keepExisting*/ true);
      dest->mDot2 = source->mDot2;
    }
  } else if (payLoadSize == 1 && dest->mDot2) {
    dest->reusePreviousHeader(dest->mDot2, source->mDot2, source->mDot2,
                              builder);
    if (!replaceOldHeader) {
      source->mDot2 = dest->mDot2;
    }
  }

  if (payLoadSize == 1) {
    // Check this function's comments for why no def-use changes
    // should be made on resetting src(0).
    G4_Operand *src0 = source->send->getSrc(0);
    dest->send->setSrc(builder.duplicateOperand(src0), 0);
  }

  dest->opt = true;

  return;
}

bool Optimizer::isHeaderCachingCandidate(G4_INST *inst) {
  if (inst->isSend()) {
    return true;
  }

  if (inst->useEmpty()) {
    return false;
  }

  for (USE_EDGE_LIST_ITER iter = inst->use_begin(), iend = inst->use_end();
       iter != iend; ++iter) {
    if ((*iter).first->isSend()) {
      G4_INST *send = (*iter).first;
      G4_Operand *header = send->getSrc(0);
      G4_DstRegRegion *dst = inst->getDst();

      // def to BuiltInA0 is part of header opt
      if (inst->getDst() && inst->getDst()->getBase() &&
          inst->getDst()->getBase()->isRegVar() &&
          inst->getDst()->getBase()->asRegVar() ==
              builder.getBuiltinA0()->getRegVar() &&
          inst->getDst()->getRegOff() == 0 &&
          inst->getDst()->getSubRegOff() == 0) {
        return true;
      }

      // make sure that dst of the current inst is header, not payload
      // header is hard-coded to be 32 bytes
      if (header->getTopDcl() == dst->getTopDcl() &&
          dst->getLeftBound() >= header->getLeftBound() &&
          dst->getRightBound() <=
              header->getLeftBound() + kernel.numEltPerGRF<Type_UB>() - 1) {
        return true;
      }
      return false;
    }
  }

  return false;
}

/*
 * mark the below "and" instruction as redundant;
 * the "and" instruction is the header for a barrier:
 *   and (8) r1.0<1>:ud r0.2<0;1,0>:ud 0xf000000:ud {Align1, NoMask}
 *   send (1) null<1>:ud r1 0x3 0x2000004:ud{Align1}
 *   wait n0:ud {Align1}
 */
void Optimizer::removeRedundantBarrierHeaders(G4_INST *sendInst,
                                              G4_SrcRegRegion *barrierSrc0,
                                              bool first) {
  bool barrier = false;
  G4_SrcRegRegion *src0 = NULL;
  if (!first) // skip the check as already done so for first barrier
  {
    barrier = isBarrierPattern(sendInst, src0);
  }
  if (barrier || first) {
    auto item = sendInst->def_begin();
    G4_INST *andInst = item->first;
    // delete all the uses of andInst
    // addInst and sendInst will have no def and no use
    andInst->removeAllUses();
    // sendInst.src0 (addInst.dst) will be replaced by barrierSend.src0
    // create a new G4_SrcRegRegion, which is a copy of barrierSend.src0
    G4_SrcRegRegion *src = builder.createSrc(
        barrierSrc0->getTopDcl()->getRegVar(), 0, 0, barrierSrc0->getRegion(),
        barrierSrc0->getTopDcl()->getElemType());
    sendInst->setSrc(src, 0);
    andInst->markDead();
  }
}

/*
 * pattern match of a code sequence for ISA_BARRIER:
 *   and (8) r1.0<1>:ud r0.2<0;1,0>:ud 0xf000000:ud {Align1, NoMask}
 *   send (1) null<1>:ud r1 0x3 0x2000004:ud{Align1}
 *   wait n0:ud {Align1}
 */
bool Optimizer::isBarrierPattern(G4_INST *sendInst,
                                 G4_SrcRegRegion *&barrierSendSrc0) {
  /*
   * check G4_send
   */
  G4_SendDescRaw *desc = sendInst->getMsgDescRaw();
  if (!desc)
    return false;
  uint32_t descVal = desc->getDesc();
  if ((desc->getFuncId() == SFID::GATEWAY) &&
      (descVal == (0x1 << 25) + 0x4) && // 0x2000004
      (sendInst->def_size() == 1)) {
    auto item = sendInst->def_begin();
    G4_INST *andInst = item->first; // getting "and" from send's def

    /*
     * check G4_and
     */
    if ((andInst) && (andInst->opcode() == G4_and) &&
        (item->second == Opnd_src0)) {
      G4_Operand *src0 = andInst->getSrc(0);
      G4_Operand *src1 = andInst->getSrc(1);

      bool isSrc0 =
          ((src0->isSrcRegRegion()) && (src0->asSrcRegRegion()->getBase()) &&
           (src0->asSrcRegRegion()->getBase()->isRegVar()) &&
           (src0->asSrcRegRegion()->getBase()->asRegVar() ==
            builder.getBuiltinR0()->getRegVar()) &&
           (src0->asSrcRegRegion()->getRegOff() == 0) &&
           (src0->asSrcRegRegion()->getSubRegOff() == 2)); // r0.2

      bool isSrc1 =
          src1->isImm() && !src1->isRelocImm() &&
          src1->asImm()->getInt() ==
              (builder.getPlatform() >= GENX_SKL ? 0x8F000000 : 0x0F000000);

      if (isSrc0 && isSrc1 && sendInst->getSrc(0) &&
          sendInst->getSrc(0)->isSrcRegRegion()) {
        barrierSendSrc0 = sendInst->getSrc(0)->asSrcRegRegion();
        return true;
      }
    }
  }

  return false;
}

/*
 * add the the barrier header as the top instruction
 */
void Optimizer::hoistBarrierHeaderToTop(G4_SrcRegRegion *barrierSendSrc0) {
  G4_Declare *dcl = barrierSendSrc0->getTopDcl();
  IR_Builder *mybuilder = &builder;

  // below code is copied from translateVISASyncInst() for ISA_BARRIER
  // all other dwords are ignored
  // and (8) r32.0:ud r0.2:ud 0x0F000000

  G4_SrcRegRegion *r0_src_opnd =
      builder.createSrc(mybuilder->getBuiltinR0()->getRegVar(), 0, 2,
                        builder.getRegionScalar(), Type_UD);
  G4_DstRegRegion *dst1_opnd =
      builder.createDst(dcl->getRegVar(), 0, 0, 1, Type_UD);

  G4_Imm *g4Imm = NULL;

  // for SKL+ there are 5 bits for barrierID
  // 5th bit is stored in bit 31 of second dword
  if (builder.getPlatform() < GENX_SKL) {
    g4Imm = builder.createImm(0x0F000000, Type_UD);
  } else {
    g4Imm = builder.createImm(0x8F000000, Type_UD);
  }

  G4_INST *andInst =
      builder.createBinOp(G4_and, g4::SIMD8, dst1_opnd, r0_src_opnd, g4Imm,
                          InstOpt_WriteEnable, false);
  for (auto bb : fg) {
    auto iter = std::find_if(bb->begin(), bb->end(),
                             [](G4_INST *inst) { return !inst->isLabel(); });
    if (iter != bb->end()) {
      bb->insertBefore(iter, andInst);
      return;
    }
  }
}

/*
 * check whether there are new definitions in order to determine redundancy
 */
bool Optimizer::chkNewDefBetweenSends(G4_INST *inst, MSGTableList &msgList,
                                      DEFA0 &myA0) {
  bool isDef = false;
  msgList.unique();

  // check SIMD8 VxH region everytime
  if (inst->getDst() &&
      (inst->getDst()->isIndirect() || inst->getDst()->isDirectAddress())) {
    isDef = myA0.isA0Redef = true;
  } else {
    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
      if (!inst->getSrc(i) || !inst->getSrc(i)->isSrcRegRegion())
        continue;
      auto src = inst->getSrc(i)->asSrcRegRegion();
      if (src->isIndirect() || src->isDirectAddress()) {
        isDef = myA0.isA0Redef = true;
        break;
      }
    }
  }

  if (msgList.size() < 2) {
    return false;
  }
  MSGTable_ITER ii = msgList.begin();
  if (ii == msgList.end()) {
    return false;
  }

  ii++;

  if (ii == msgList.end()) {
    return false;
  }

  MSGTable *last = *(ii);
  if (last == NULL || last->send == NULL) {
    return false;
  }
  G4_Operand *def = inst->getDst();
  if (def == NULL) {
    return false;
  }
  if (last->mDot0 &&
      (def->compareOperand(last->mDot0->getSrc(0), builder) == Rel_eq ||
       (last->mDot0->getSrc(1) &&
        def->compareOperand(last->mDot0->getSrc(1), builder) == Rel_eq) ||
       (last->mDot0->getSrc(2) &&
        def->compareOperand(last->mDot0->getSrc(2), builder) == Rel_eq))) {
    isDef = last->isXRedef = true;
  } else if (last->mDot1 &&
             (def->compareOperand(last->mDot1->getSrc(0), builder) == Rel_eq ||
              (last->mDot1->getSrc(1) &&
               def->compareOperand(last->mDot1->getSrc(1), builder) ==
                   Rel_eq) ||
              (last->mDot1->getSrc(2) &&
               def->compareOperand(last->mDot1->getSrc(2), builder) ==
                   Rel_eq))) {
    isDef = last->isYRedef = true;
  } else if (last->mDot2 &&
             (def->compareOperand(last->mDot2->getSrc(0), builder) == Rel_eq ||
              (last->mDot2->getSrc(1) &&
               def->compareOperand(last->mDot2->getSrc(1), builder) ==
                   Rel_eq) ||
              (last->mDot2->getSrc(2) &&
               def->compareOperand(last->mDot2->getSrc(2), builder) ==
                   Rel_eq))) {
    isDef = last->isSizeRedef = true;
  } else if (last->m &&
             (def->compareOperand(last->m->getSrc(0), builder) == Rel_eq ||
              (last->m->getSrc(1) &&
               def->compareOperand(last->m->getSrc(1), builder) == Rel_eq) ||
              (last->m->getSrc(2) &&
               def->compareOperand(last->m->getSrc(2), builder) == Rel_eq))) {
    isDef = last->isR0Dot0Redef = true;
  }
  return isDef;
}

/*
 * Cache the send and its def into a table for optimization
 */
void Optimizer::addEntryToMessageTable(G4_INST *inst, MSGTableList &msgList,
                                       G4_BB *bb, INST_LIST_ITER ii,
                                       DEFA0 &myA0) {
  MSGTable *item = msgList.front();
  if (inst->isSend()) {
    item->send = inst;
    item->opt = false;
    item->isR0Dot0Redef = false;
    item->isXRedef = false;
    item->isYRedef = false;
    item->isSizeRedef = false;

    if (item->invalid) {
      msgList.pop_front();
    } else if (item->a0Dot0 != NULL && // only def a0.0
               (item->m == NULL || item->mDot2 == NULL)) {
      if (isHeaderOptCandidate(item->a0Dot0, myA0.pred)) {
        if (isHeaderOptReuse(item->a0Dot0, myA0.pred) && !myA0.isA0Redef) {
          // Transfer uses of a0Dot0 to myA0.pred. This removes uses from
          // a0Dot0 and add to myA0.pred. item->a0Dot0 to be deleted.
          item->a0Dot0->transferUse(myA0.pred, /*keepExisting*/ true);
          item->a0Dot0->markDead();
        }
      }
      msgList.pop_front();
    } else if (item->a0Dot0 && item->m && item->mDot2) // complete header def
    {

      msgList.unique();
      if (msgList.size() >= 2) {
        optMessageHeaders(msgList, bb, myA0);
        if (msgList.front()->opt &&
            msgList.front()->send->getMsgDesc()->getSrc0LenRegs() == 1) {
          // keep the oldest send for subsequent read operations
          // but the instruction to define a0.0 needs to be latest
          // msgList.back()->a0Dot0 = msgList.front()->a0Dot0;
          msgList.pop_front(); // delete first element
        } else if (msgList.front()->opt &&
                   msgList.front()->send->getMsgDesc()->getSrc0LenRegs() >= 1) {
          // keep the latest send for subsequent write operations
          msgList.pop_back();
        } else {
          msgList.pop_back();
        }
        myA0.isA0Redef = false;
      }
    } else {
      // not an optimization candidate
      msgList.pop_front();
    }
  } else if (inst->getDst() && inst->getDst()->getBase() &&
             inst->getDst()->getBase()->isRegVar() &&
             inst->getDst()->getBase()->asRegVar() ==
                 builder.getBuiltinA0()->getRegVar() &&
             inst->getDst()->getRegOff() == 0 &&
             inst->getDst()->getSubRegOff() == 0) {
    // is builtInA0.0
    item->a0Dot0 = inst;
    item->a0Dot0_it = ii;

    if (myA0.curr == NULL) {
      myA0.pred = NULL;
      myA0.isA0Redef = false;
    } else if (!myA0.curr->isDead()) {
      // only update the a0 def when we didn't remove it
      myA0.pred = myA0.curr;
      myA0.predIt = myA0.currIt;
    }
    myA0.currIt = ii;
    myA0.curr = inst;
  } else if (inst->getSrc(0)) {
    G4_DstRegRegion *dst = inst->getDst();
    if (dst) {
      if (dst->getRegOff() == 0) {
        // mov(8) m.0, builtInR0.0
        G4_Operand *src = inst->getSrc(0);
        if (dst->getSubRegOff() == 0 && inst->getExecSize() == g4::SIMD8 &&
            src && src->isSrcRegRegion() && src->asSrcRegRegion()->getBase() &&
            src->asSrcRegRegion()->getBase()->isRegVar() &&
            src->asSrcRegRegion()->getBase()->asRegVar() ==
                builder.getBuiltinR0()->getRegVar() &&
            src->asSrcRegRegion()->getRegOff() == 0 &&
            src->asSrcRegRegion()->getSubRegOff() == 0) {
          if (item->first == HEADER_UNDEF)
            item->first = HEADER_FULL_REGISTER;
          item->m = inst;
          item->m_it = ii;
        }
        // mov(1) m.0
        else if (dst->getSubRegOff() == 0 && inst->getExecSize() == g4::SIMD1) {
          if (item->first == HEADER_UNDEF)
            item->first = HEADER_X;
          item->mDot0 = inst;
          item->mDot0_it = ii;
        }
        // mov(1) m.1
        else if (dst->getSubRegOff() == 1 && inst->getExecSize() == g4::SIMD1) {
          if (item->first == HEADER_UNDEF)
            item->first = HEADER_Y;
          item->mDot1 = inst;
          item->mDot1_it = ii;
        }
        // mov(1) m0.2
        else if (dst->getSubRegOff() == 2 && inst->getExecSize() == g4::SIMD1) {
          if (item->first == HEADER_UNDEF)
            item->first = HEADER_SIZE;
          item->mDot2 = inst;
          item->mDot2_it = ii;
        } else {
          // unrecognized update to header
          item->invalid = true;
        }
      }
    }
  }
}

void Optimizer::messageHeaderReport(size_t ic_before, size_t ic_after,
                                    G4_Kernel &kernel) {
  VISA_DEBUG({
    std::cout << "             === Message Header Optimization ===\n";
    std::cout << std::fixed << "\n";
    std::cout << kernel.getName() << " is reduced from " << ic_before << " to "
              << ic_after << " instructions.\n";
    if (((float)(ic_before)) != 0.0) {
      std::cout << std::setprecision(0)
                << (float)((ic_before - ic_after) * 100) / (float)(ic_before)
                << "% instructions of this kernel are removed.\n";
    }
    std::cout << "\n";
  });
}

//
// optimizer for removal of redundant message header instructions
//
void Optimizer::cleanMessageHeader() {
  MSGTableList msgList;
  size_t ic_before = 0;
  size_t ic_after = 0;

  llvm::SpecificBumpPtrAllocator<MSGTable> MSGTableAlloc;
  bool isRedundantBarrier = false;
  G4_SrcRegRegion *barrierSendSrc0 = nullptr;

  for (G4_BB *bb : fg) {
    msgList.clear();
    auto MSGTableMem = MSGTableAlloc.Allocate();
    MSGTable *newItem = new (MSGTableMem) MSGTable();
    newItem->first = HEADER_UNDEF;

    msgList.push_front(newItem);
    INST_LIST_ITER ii = bb->begin();
    INST_LIST_ITER iend = bb->end();
    ic_before += bb->size();

    DEFA0 myA0;
    myA0.curr = nullptr;
    myA0.pred = nullptr;
    myA0.isA0Redef = false;

    for (; ii != iend; ii++) {
      G4_INST *inst = *ii;
      if (isHeaderCachingCandidate(inst)) {
        if (inst->opcode() == G4_send && isRedundantBarrier) {
          removeRedundantBarrierHeaders(inst, barrierSendSrc0, false);
        } else if (inst->opcode() == G4_send && !isRedundantBarrier) {
          isRedundantBarrier = isBarrierPattern(inst, barrierSendSrc0);
          if (isRedundantBarrier) {
            removeRedundantBarrierHeaders(inst, barrierSendSrc0, true);
          }
        }

        addEntryToMessageTable(inst, msgList, bb, ii, myA0);
        if (inst->isSend()) {
          auto MSGTableMem = MSGTableAlloc.Allocate();
          MSGTable *item = new (MSGTableMem) MSGTable();
          item->first = HEADER_UNDEF;
          msgList.push_front(item);
        }
      } else {
        chkNewDefBetweenSends(inst, msgList, myA0);
      }
    }

    // Dead code elimination
    for (ii = bb->begin(); ii != bb->end();) {
      G4_INST *inst = *ii;
      INST_LIST_ITER curr = ii++;
      if (inst->isDead()) {
        inst->removeUseOfInst();
        bb->erase(curr);
      }
    }

    ic_after += bb->size();
  }

  messageHeaderReport(ic_before, ic_after, kernel);

  if (isRedundantBarrier) {
    hoistBarrierHeaderToTop(barrierSendSrc0);
  }
  msgList.clear();
}
//  The end of message header optimization

// For NoMask inst with non-zero mask offset, set maskoffset = 0 if possible.
//
// For any NoMask inst with non-zero mask offset, if it does not access any
// ARF and it is not a CF instruction, set its mask offset to zero.
void Optimizer::forceNoMaskOnM0() {
  for (G4_BB *currBB : fg) {
    for (auto &I : *currBB) {
      if (!I->isWriteEnableInst() || I->isCFInst() || I->getPredicate() ||
          I->getCondMod() || I->getMaskOffset() == 0 ||
          I->hasImplicitAccDst() || I->hasImplicitAccSrc())
        continue;

      // skip if I is logical on flag registers.
      // For example:
      //   (W) pseudo_and (16|M16)  P2:uw  P2:uw  P1:uw
      // where P2 is the high 16 bits of a 32-bit flag
      // and M16 cannot be changed to M0.
      if (I->isLogic() || I->isPseudoLogic()) {
        // Only checking dst is enough.
        G4_DstRegRegion *dst = I->getDst();
        if (dst && !dst->isNullReg() && dst->isFlag()) {
          continue;
        }
      }

      I->setMaskOption(InstOpt_M0);
    }
  }
}

void Optimizer::sendFusion() {
  // Potential problem related to noMask WA
  //
  // Send fusion creates the following code:
  //   1. (W)  mov (1|M0)             f1.0<1>:uw   0x0:uw
  //   2.      cmp (8|M0)  (eq)f1.0   null<1>:uw   r0.0<8;8,1>:uw r0.0<8;8,1>:uw
  //   3. (W)  mov (1|M0)             r18.4<1>:uw  f1.0<0;1,0>:uw
  //   4. (W)  mov (2|M0)             r18.8<1>:ub  r18.8<0;1,0>:ub
  //   5. (W)  mov (1|M0)             f0.1<1>:uw   r18.4<0;1,0>:uw
  //      fused send:
  //      (W&f0.1) send.dc1 (16|M0)   r5   r27  r1   0x40      0x02205EFF
  //
  // This code also works if NoMask WA is needed. Actually, this f0.1 behaves
  // the same as NoMask WA.  And it is critical that all of them should be
  // executed without applying NoMask WA. Here is the reason why:
  //     Assume we have a HW bug, no channels are on but it runs thru those
  //     instructions. We have f1.0 be all 0 at the end of 2.  As result, f0.1
  //     will be all zero. And the fused send will not run as its predicate is
  //     false. But if NoMask WA applies to 3 in postRA WA as it thinks it is a
  //     flag spill. (3) becomes:
  //           (3)  (W& f0.0.any8)  mov (1|M0)   r18.4<1>:uw f1.0<0;1,0>:uw
  //     therefore,  this instruction will no longer run, as result, f0.1 has
  //     garbage and it may make the fused send to run, which is wrong.
  //
  // The solutions:
  //  1) turn off send fusion (does it really help?);
  //  2) don't apply WA on those instructions.
  // As those 1-5 are all local definitions, postRA WA should skip them.
  // For now, we will do 2 to minimize potential impacts.
  if (builder.hasFusedEU()) {
    // Turn off send fusion for EU Fusion platforms.
    return;
  }
  (void)doSendFusion(&fg, &mem);
}

G4_SrcRegRegion *IR_Builder::createSubSrcOperand(G4_SrcRegRegion *src,
                                                 uint16_t start, uint8_t size,
                                                 uint16_t newVs,
                                                 uint16_t newWd) {
  const RegionDesc *rd = NULL;
  uint16_t vs = src->getRegion()->vertStride, hs = src->getRegion()->horzStride,
           wd = src->getRegion()->width;
  G4_Type srcType = src->getType();
  // even if src has VxH region, it could have a width that is equal to the new
  // exec_size, meaning that it's really just a 1x1 region.
  auto isVxHRegion = src->getRegion()->isRegionWH() && wd < size;
  if (!isVxHRegion) {
    // r[a0.0,0]<4;2,1> and size is 4 or 1
    if (size < newWd) {
      newWd = size;
    }
    rd = size == 1
             ? getRegionScalar()
             : createRegionDesc(size == newWd ? newWd * hs : newVs, newWd, hs);
    rd = getNormalizedRegion(size, rd);
  }

  if (src->getRegAccess() != Direct) {
    if (isVxHRegion) {
      // just handle <1,0>
      if (start > 0) {
        // Change a0.N to a0.(N+start)
        vISA_ASSERT((start % wd == 0),
                    "illegal starting offset and width combination");
        uint16_t subRegOff = src->getSubRegOff() + start / wd;
        return createIndirectSrc(src->getModifier(), src->getBase(),
                                 src->getRegOff(), subRegOff, src->getRegion(),
                                 src->getType(), src->getAddrImm());
      } else {
        return duplicateOperand(src);
      }
    }

    if (start > 0) {
      short numRows = start / wd;
      short numCols = start % wd;
      short newOff = (numRows * vs + numCols * hs) * TypeSize(srcType);
      auto newSrc = createIndirectSrc(
          src->getModifier(), src->getBase(), src->getRegOff(),
          src->getSubRegOff(), rd, src->getType(), src->getAddrImm() + newOff);
      return newSrc;
    } else {
      G4_SrcRegRegion *newSrc = duplicateOperand(src);
      newSrc->setRegion(*this, rd);
      return newSrc;
    }
  }

  // direct access oprand
  if (src->isAccReg()) {
    switch (srcType) {
    case Type_F:
      // must be acc1.0 as result of simd16 -> 8 split
      vISA_ASSERT(size == 8, "only support simd16->simd8 for now");
      return createSrcRegRegion(src->getModifier(), Direct,
                                phyregpool.getAcc1Reg(), 0, 0, src->getRegion(),
                                srcType);
    case Type_HF: {
      // can be one of acc0.8, acc1.0, acc1.8
      if (src->getBase()->asAreg()->getArchRegType() == AREG_ACC1) {
        start += 16;
      }
      G4_Areg *accReg =
          start >= 16 ? phyregpool.getAcc1Reg() : phyregpool.getAcc0Reg();
      return createSrcRegRegion(src->getModifier(), Direct, accReg, 0,
                                start % 16, src->getRegion(), srcType);
    }
    default:
      // Keep using acc0 for other types.
      return duplicateOperand(src);
    }
  }

  // Since this function creates a new sub src operand based on a start offset,
  // the reg and subreg offsets need to be re-computed.
  uint16_t regOff, subRegOff, subRegOffByte, newSubRegOffByte, newEleOff,
      newEleOffByte, crossGRF;

  newEleOff =
      start * hs +
      (start >= wd && vs != wd * hs ? (start / wd * (vs - wd * hs)) : 0);

  // Linearize offsets into bytes to verify potential GRF crossing
  newEleOffByte = newEleOff * src->getTypeSize();
  subRegOffByte = src->getSubRegOff() * src->getTypeSize();

  // If subreg crosses GRF size, update reg and subreg offset accordingly
  newSubRegOffByte = subRegOffByte + newEleOffByte;
  crossGRF = newSubRegOffByte / kernel.numEltPerGRF<Type_UB>();

  newSubRegOffByte =
      newSubRegOffByte - crossGRF * kernel.numEltPerGRF<Type_UB>();

  // Compute final reg and subreg offsets
  regOff = src->getRegOff() + crossGRF;
  subRegOff = newSubRegOffByte / src->getTypeSize();

  return createSrcRegRegion(src->getModifier(), Direct, src->getBase(), regOff,
                            subRegOff, rd, srcType, src->getAccRegSel());
}

G4_DstRegRegion *IR_Builder::createSubDstOperand(G4_DstRegRegion *dst,
                                                 uint16_t start, uint8_t size) {
  if (dst->getRegAccess() != Direct) {
    if (start > 0) {
      // just change immediate offset
      uint16_t newOff = start * dst->getTypeSize() * dst->getHorzStride();
      G4_DstRegRegion *newDst = duplicateOperand(dst);
      newDst->setImmAddrOff(dst->getAddrImm() + newOff);
      return newDst;
    } else {
      return duplicateOperand(dst);
    }
  }

  uint16_t regOff, subRegOff;
  if (start > 0) {
    G4_Type dstType = dst->getType();
    uint16_t hs = dst->getHorzStride();
    if (dst->isAccReg()) {
      switch (dstType) {
      case Type_F:
        // must be acc1.0 as result of simd16 -> 8 split
        vISA_ASSERT(size == 8, "only support simd16->simd8 for now");
        return createDst(phyregpool.getAcc1Reg(), 0, 0, hs, dstType);
      case Type_HF: {
        // can be one of acc0.8, acc1.0, acc1.8
        if (dst->getBase()->asAreg()->getArchRegType() == AREG_ACC1) {
          start += 16;
        }
        G4_Areg *accReg =
            start >= 16 ? phyregpool.getAcc1Reg() : phyregpool.getAcc0Reg();
        return createDst(accReg, 0, start % 16, hs, dstType);
      }
      default:

        // other types do not support acc1, we have to continue to use acc0
        // whoever doing the split must fix the dependencies later by shuffling
        // instructions so that acc0 does not get overwritten
        return createDstRegRegion(*dst);
      }
    }

    // Linearize offsets into bytes to verify potential GRF crossing
    uint16_t newSubRegOff, newSubRegOffByte, crossGRF;

    newSubRegOff = dst->getSubRegOff() + start * hs;
    newSubRegOffByte = newSubRegOff * TypeSize(dstType);

    crossGRF = newSubRegOffByte / kernel.numEltPerGRF<Type_UB>();
    newSubRegOffByte =
        newSubRegOffByte - crossGRF * kernel.numEltPerGRF<Type_UB>();

    // Compute final reg and subreg offsets
    regOff = dst->getRegOff() + crossGRF;
    subRegOff = newSubRegOffByte / TypeSize(dstType);

    // create a new one
    return createDst(dst->getBase(), regOff, subRegOff, hs, dst->getType(),
                     dst->getAccRegSel());
  } else {
    G4_DstRegRegion *newDst = duplicateOperand(dst);
    return newDst;
  }
}

G4_INST *IR_Builder::makeSplittingInst(G4_INST *inst, G4_ExecSize ExSize) {
  // Instruction's option is reused. Call sites should reset this field
  // properly. FIXME: fix all call sites.
  G4_INST *newInst = NULL;
  G4_opcode op = inst->opcode();
  if (inst->isMath()) {
    newInst = createMathInst(NULL, inst->getSaturate(), ExSize, NULL, NULL,
                             NULL, inst->asMathInst()->getMathCtrl(),
                             inst->getOption(), true);
  } else if (inst->getNumSrc() < 3) {
    newInst = createInternalInst(NULL, op, NULL, inst->getSaturate(), ExSize,
                                 NULL, NULL, NULL, inst->getOption());
  } else {
    newInst = createInternalInst(NULL, op, NULL, inst->getSaturate(), ExSize,
                                 NULL, NULL, NULL, NULL, inst->getOption());
  }

  newInst->inheritDIFrom(inst);

  return newInst;
}

// HW WAs that are done before RA.
void Optimizer::preRA_HWWorkaround() {
  if (builder.hasFusedEUNoMaskWA()) {
    prepareNoMaskWA();
  }

  // Call WA for fused EU
  if (builder.hasFusedEU() && kernel.hasIndirectCall()) {
    applyFusedCallWA();
    // Reset pre- and post-Id after FusedCallWA, which may add new
    // basic blocks.
    kernel.fg.reassignBlockIDs();
    kernel.fg.findBackEdges();
  }

  insertFenceAtEntry();

  cloneSampleInst();

  insertIEEEExceptionTrap();

  if (builder.supportNativeSIMD32())
    fixDirectAddrBoundOnDst();
}

//
// HW WAs that are done right after RA.
//    Sometime, a WA needs both preRA and postRA WA and postRA needs info from
//    preRA (NoMask WA). If doing postWA in HWWorkaround,  some instructions, or
//    even basic blocks (ifcvt), are removed, which could interfere information
//    passing from preRA to postRA. The loss of such the interference can cause
//    postRA WA to fail.  For this purpose, a postRA_HWWorkaround is added. This
//    also means that BBs and insts between preRA pass and postRA pass remain
//    undeleted (is it too strong?).
//
//    Note that for those WAs that should be done after inst scheduling, they
//    should go to HWWorkaround, not here.
//
void Optimizer::postRA_HWWorkaround() {
  if (builder.hasFusedEUNoMaskWA()) {
    applyNoMaskWA();
  }
  if (builder.supportNativeSIMD32())
    fixDirectAddrBoundOnDst();
}

// should only be called post-RA, return true if this operand has overlapping
// GRF with other ToDo: extend to non-GRF operands?
static bool hasOverlappingGRF(G4_Operand *opnd, G4_Operand *other) {
  if (!opnd || !other || !opnd->isGreg() || !other->isGreg())
    return false;
  auto LB = opnd->getLinearizedStart(), RB = opnd->getLinearizedEnd();
  auto otherLB = other->getLinearizedStart(),
       otherRB = other->getLinearizedEnd();
  return !(RB < otherLB || LB > otherRB);
}

// returns for this fence instruction the iterator position where the commit
// move should be inserted. We conservatively assume a commit is needed before
// -- another send
// -- any optimization barrier
// -- any instruction that writes to fence's dst GRF
// If another instruction happens to read dst GRF, then it serves as the commit
// and we don't need the dummy move
std::optional<INST_LIST_ITER>
Optimizer::findFenceCommitPos(INST_LIST_ITER fence, G4_BB *bb) const {
  auto fenceInst = *fence;
  vASSERT(fenceInst->isSend() && fenceInst->asSendInst()->isFence());
  auto dst = fenceInst->getDst();
  auto I = std::next(fence);
  for (auto E = bb->end(); I != E; ++I) {
    G4_INST *inst = *I;
    if (inst->isSend() || inst->isCFInst() || inst->isLabel() ||
        inst->isOptBarrier()) {
      break;
    }
    if (hasOverlappingGRF(dst, inst->getDst())) {
      break;
    }
    for (auto SI = inst->src_begin(), SE = inst->src_end(); SI != SE; ++SI) {
      auto src = *SI;
      if (hasOverlappingGRF(dst, src)) {
        return std::nullopt;
      }
    }
  }
  return I;
}

bool Optimizer::addFenceCommit(INST_LIST_ITER ii, G4_BB *bb,
                               bool scheduleFenceCommit) {
  G4_INST *inst = *ii;
  G4_InstSend *sendInst = inst->asSendInst();
  vASSERT(sendInst);
  if (sendInst && sendInst->getMsgDesc()->getDstLenRegs() > 0) {
    // commit is enabled for the fence, need to generate a move after to make
    // sure the fence is complete mov (8) r1.0<1>:ud r1.0<8;8,1>:ud {NoMask}
    auto nextIter = std::next(ii);
    if (scheduleFenceCommit) {
      auto iter = findFenceCommitPos(ii, bb);
      if (!iter) {
        return false; // skip commit for this fence
      }
      nextIter = *iter;
    }
    auto dst = inst->getDst();
    G4_Declare *fenceDcl = dst->getBase()->asRegVar()->getDeclare();
    G4_DstRegRegion *movDst = builder.createDst(
        builder.phyregpool.getNullReg(), 0, 0, 1, fenceDcl->getElemType());
    G4_SrcRegRegion *movSrc =
        builder.createSrcRegRegion(fenceDcl, builder.createRegionDesc(8, 8, 1));
    G4_INST *movInst = builder.createMov(g4::SIMD8, movDst, movSrc,
                                         InstOpt_WriteEnable, false);
    movInst->addComment("memory fence commit");
    bb->insertBefore(nextIter, movInst);
  } else if (builder.hasFenceControl()) {
    // null dst, use sync.fence instead
    auto nextIter = std::next(ii);
    G4_INST *syncInst = builder.createInternalInst(
        nullptr, G4_sync_fence, nullptr, g4::NOSAT, g4::SIMD1, nullptr,
        builder.createNullSrc(Type_UD), nullptr, InstOpt_NoOpt);
    bb->insertBefore(nextIter, syncInst);
  }
  return true;
}

//
// rewrite source regions to satisfy various HW requirements.  This pass will
// not modify the instructions otherwise
// -- rewrite <1;1,0> to <2;2,1> when possible (exec size > 1, width is not used
// to cross GRF)
//    as HW doesn't allow <1;1,0> to be co-issued
//
void Optimizer::normalizeRegion() {
  for (auto bb : fg) {
    for (auto inst : *bb) {
      if (inst->isCall() || inst->isFCall() ||
          inst->isReturn() || inst->isFReturn()) {
        // Do not rewrite region for call or return,
        // as the effective execution size is 2.
        continue;
      }

      // Do not rewrite region for dpas as HW requires region <1;1,0>
      if (inst->isDpas())
        continue;

      if (inst->getExecSize() == g4::SIMD1) {
        // Replace: mov (1) r64.0<4>:df  r3.0<0;1,0>:df
        // with:    mov (1) r64.0<1>:df  r3.0<0;1,0>:df
        // otherwise, will get incorrect results for HSW, HW mode
        G4_Operand *dst = inst->getDst();
        if (dst != NULL && dst->asDstRegRegion()->getHorzStride() > 1 &&
            dst->getTypeSize() == 8) {
          dst->asDstRegRegion()->setHorzStride(1);
        }
      } else {
        for (int i = 0; i < inst->getNumSrc(); ++i) {
          G4_Operand *src = inst->getSrc(i);
          // Only rewrite direct regions.
          if (src && src->isSrcRegRegion() &&
              src->asSrcRegRegion()->getRegAccess() == Direct) {
            G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
            if (srcRegion->getRegion()->isContiguous(inst->getExecSize())) {
              srcRegion->rewriteContiguousRegion(builder, i);
            } else if (inst->isAlign1Ternary()) {
              // special checks for 3src inst with single non-unit stride region
              // rewrite it as <s*2;s>
              uint16_t stride = 0;
              if (srcRegion->getRegion()->isSingleNonUnitStride(
                      inst->getExecSize(), stride)) {
                vISA_ASSERT(stride <= 4,
                            "illegal stride for align1 ternary region");
                srcRegion->setRegion(
                    builder,
                    kernel.fg.builder->createRegionDesc(stride * 2, 2, stride));
              }
            }
          }
        }
      }
    }
  }
}

void Optimizer::countGRFUsage() {
  unsigned int maxGRFNum = kernel.getNumRegTotal();
  int count = 0;
  std::vector<bool> GRFUse(maxGRFNum, false);
  for (auto dcl : kernel.Declares) {
    if (!fg.getHasStackCalls() &&
        (builder.isPreDefFEStackVar(dcl) || builder.isPreDefSpillHeader(dcl))) {
      continue;
    }
    if (dcl->getRegVar()->isGreg()) {
      int GRFStart = dcl->getRegVar()->getPhyReg()->asGreg()->getRegNum();
      int numRows = dcl->getNumRows();
      vISA_ASSERT(GRFStart >= 0 && (GRFStart + numRows) <= (int)maxGRFNum,
                  "illegal GRF assignment");
      for (int i = GRFStart; i < GRFStart + numRows; ++i) {
        GRFUse[i] = true;
      }
    }
  }
  for (unsigned int i = 0; i < maxGRFNum; ++i)
    if (GRFUse[i])
      count++;
  fg.builder->getJitInfo()->stats.numGRFUsed = count;
  fg.builder->criticalMsgStream()
      << "\tKernel " << kernel.getName() << " : " << count << " registers\n";
}

//
//  Dump the input payload to start of scratch space.
//  this is strictly for debugging and we do not care if this gets overwritten
//  by other usage of the scratch space (private memory, spill, etc.)
//
void Optimizer::dumpPayload() {
  int inputEnd = 0;
  for (int i = 0, numInput = kernel.fg.builder->getInputCount(); i < numInput;
       ++i) {
    input_info_t *input_info = kernel.fg.builder->getInputArg(i);
    if (inputEnd < input_info->size + input_info->offset) {
      inputEnd = input_info->size + input_info->offset;
    }
  }

  G4_BB *bb = kernel.fg.getEntryBB();
  // iter points to the first non-label inst
  auto iter = bb->begin(), bbEnd = bb->end();
  while (iter != bbEnd) {
    if (!(*iter)->isLabel()) {
      break;
    }
    ++iter;
  }

  int regOffset = (inputEnd + kernel.numEltPerGRF<Type_UB>() - 1) /
                  kernel.numEltPerGRF<Type_UB>();

  static const unsigned SCRATCH_MSG_DESC_CATEGORY = 18;
  static const unsigned SCRATCH_MSG_DESC_OPERATION_MODE = 17;
  static const unsigned SCRATCH_MSG_DESC_CHANNEL_MODE = 16;
  static const unsigned SCRATCH_MSG_DESC_BLOCK_SIZE = 12;

  // write 8 GRFs at a time
  int msgSize = 8;
  for (int i = 1; i < regOffset; i += msgSize) {
    uint16_t extFuncCtrl = 0;
    // both scratch and block read use DC
    SFID funcID = SFID::DP_DC0;

    uint32_t headerPresent = 0x80000;
    uint32_t msgDescImm = headerPresent;
    uint32_t msgLength = 1;
    uint32_t blocksizeEncoding = 0x3; // 8 GRF
    msgDescImm |= (msgLength << getSendMsgLengthBitOffset());
    msgDescImm |= (1 << SCRATCH_MSG_DESC_CATEGORY);
    msgDescImm |= (1 << SCRATCH_MSG_DESC_CHANNEL_MODE);
    msgDescImm |= (1 << SCRATCH_MSG_DESC_OPERATION_MODE);

    msgDescImm |= (blocksizeEncoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
    msgDescImm |= i;

    G4_SendDescRaw *desc = kernel.fg.builder->createSendMsgDesc(
        msgDescImm, 0, 1, funcID, msgSize, extFuncCtrl, SendAccess::WRITE_ONLY);
    const RegionDesc *region = kernel.fg.builder->getRegionStride1();
    G4_SrcRegRegion *headerOpnd = kernel.fg.builder->createSrcRegRegion(
        kernel.fg.builder->getBuiltinR0(), region);
    G4_Declare *tempDcl =
        builder.createHardwiredDeclare(msgSize * 8, Type_UD, i, 0);
    G4_SrcRegRegion *srcOpnd =
        kernel.fg.builder->createSrcRegRegion(tempDcl, region);
    G4_DstRegRegion *dstOpnd = kernel.fg.builder->createNullDst(Type_UD);

    G4_INST *sendInst = kernel.fg.builder->createSplitSendInst(
        nullptr, G4_sends, g4::SIMD16, dstOpnd, headerOpnd, srcOpnd,
        kernel.fg.builder->createImm(msgDescImm, Type_UD), InstOpt_WriteEnable,
        desc, nullptr, true);
    bb->insertBefore(iter, sendInst);
  }
}

// perform simple stat collection (e.g., numSends)
// IR is not modified
void Optimizer::collectStats() {
  uint32_t numSends = 0;
  for (auto bb : fg) {
    for (auto inst : *bb) {
      if (inst->isSend()) {
        numSends++;
      }
      if (!builder.hasDFInst() && inst->isDFInstruction()) {
        builder.setHasDFInst(true);
      }
    }
  }
}

void Optimizer::mapOrphans() {
  auto catchAllCISAOff = builder.debugInfoPlaceholder;
  if (catchAllCISAOff == UNMAPPABLE_VISA_INDEX)
    return;

  for (auto bb : kernel.fg) {
    for (auto inst : *bb) {
      if (inst->getVISAId() == UNMAPPABLE_VISA_INDEX) {
        inst->setVISAId(catchAllCISAOff);
      }
    }
  }
}

G4_Declare *Optimizer::createInstsForCallTargetOffset(InstListType &insts,
                                                      G4_INST *fcall,
                                                      int64_t adjust_off) {
  // create instruction sequence:
  //       add  r2.0  -IP   call_target
  //       add  r2.0  r2.0  adjust_off

  // call's dst must be r125.0, which is reserved at
  // GlobalRA::setABIForStackCallFunctionCalls.
  vASSERT(fcall->getDst()->isGreg());
  // call dst must not be overlapped with r2 which is hardcoded as the new jump
  // target
  vASSERT((fcall->getDst()->getLinearizedStart() /
           kernel.numEltPerGRF<Type_UB>()) != 2);

  // hardcoded add's dst to r2
  // the reg offset must be the same as call's dst reg, and must be 0 (HW
  // restriction)
  uint32_t reg_off = fcall->getDst()->getLinearizedStart() %
                     kernel.numEltPerGRF<Type_UB>() /
                     fcall->getDst()->getTypeSize();

  G4_Declare *add_dst_decl =
      builder.createHardwiredDeclare(1, fcall->getDst()->getType(), 2, reg_off);

  // create the first add instruction
  // add  r2.0  -IP   call_target
  G4_INST *add_inst = builder.createBinOp(
      G4_add, g4::SIMD1, builder.createDstRegRegion(add_dst_decl, 1),
      builder.createSrcRegRegion(Mod_Minus, Direct,
                                 builder.phyregpool.getIpReg(), 0, 0,
                                 builder.getRegionScalar(), Type_UD),
      fcall->getSrc(0), InstOpt_WriteEnable | InstOpt_NoCompact, false);

  if (builder.needIPWA())
    replaceIPWithCall(insts, add_inst);

  // create the second add to add the -ip to adjust_off, adjust_off dependes
  // on how many instructions from the fist add to the jmp instruction, and
  // if it's post-increment (jmpi) or pre-increment (call)
  // add  r2.0  r2.0  adjust_off
  G4_INST *add_inst2 = builder.createBinOp(
      G4_add, g4::SIMD1, builder.createDstRegRegion(add_dst_decl, 1),
      builder.createSrcRegRegion(add_dst_decl, builder.getRegionScalar()),
      builder.createImm(adjust_off, Type_D),
      InstOpt_WriteEnable | InstOpt_NoCompact, false);

  insts.push_back(add_inst);
  insts.push_back(add_inst2);

  return add_dst_decl;
}

void Optimizer::replaceIPWithCall(InstListType &insts, G4_INST *inst_with_ip) {
  // Expand
  //    add    dst      -IP   call_target
  // To
  //    call   dst     _label_ip_wa          // jump to the next instruction
  //  _label_ip_wa:
  //    add    dst     dst     32            // adjust dst to the next 2
  //    instruction's ip ret    dst                           // jump to the
  //    next instruction add    dst     -dst    call_target   // at this
  //    instruction dst is the ip value

  uint32_t reg_num = inst_with_ip->getDst()->getLinearizedStart() /
                     kernel.numEltPerGRF<Type_UB>();
  uint32_t reg_off = inst_with_ip->getDst()->getLinearizedStart() %
                     kernel.numEltPerGRF<Type_UB>() /
                     inst_with_ip->getDst()->getTypeSize();
  // call's dst must have sub-reg num 0 (HW restriction)
  vASSERT(reg_off == 0);
  G4_Declare *dst_decl = builder.createHardwiredDeclare(
      1, inst_with_ip->getDst()->getType(), reg_num, reg_off);

  // call   dst     _label_ip_wa
  // NOTE: create the call and label instructions directly without forming a BB
  // to skip the BB end with call checking (e.g. in SWSB setting) that this is
  // just a fall-throug call and is a temporarily WA
  G4_Label *label = builder.createLocalBlockLabel("ip_wa");
  insts.push_back(builder.createInternalInst(
      nullptr, G4_call, nullptr, g4::NOSAT, g4::SIMD1,
      builder.createDstRegRegion(dst_decl, 1), label, nullptr,
      InstOpt_WriteEnable));
  // _label_ip_wa:
  insts.push_back(builder.createLabelInst(label, false));

  // add    dst     dst     32
  insts.push_back(builder.createBinOp(
      G4_add, g4::SIMD1, builder.createDstRegRegion(dst_decl, 1),
      builder.createSrcRegRegion(dst_decl, builder.getRegionScalar()),
      builder.createImm(32, Type_D), InstOpt_WriteEnable | InstOpt_NoCompact,
      false));

  // ret    dst
  insts.push_back(builder.createInternalInst(
      nullptr, G4_return, nullptr, g4::NOSAT, g4::SIMD1, nullptr,
      builder.createSrcRegRegion(dst_decl, builder.getRegionScalar()), nullptr,
      InstOpt_WriteEnable | InstOpt_NoCompact));

  // update given add instruction's src0 if needed
  if (inst_with_ip->opcode() == G4_add) {
    G4_SrcRegRegion *new_src =
        builder.createSrcRegRegion(dst_decl, builder.getRegionScalar());
    new_src->setModifier(Mod_Minus);
    inst_with_ip->setSrc(new_src, 0);
  }
}

void Optimizer::createInstForJmpiSequence(InstListType &insts, G4_INST *fcall) {
  // SKL workaround for indirect call
  // r125.0 is the return IP (the instruction right after jmpi)
  // r125.1 is the return mask. While we'll replace the ret in callee to jmpi as
  // well, we do not need to consider the return mask here.

  // Do not allow predicate call on jmpi WA
  vASSERT(fcall->getPredicate() == nullptr);

  // calculate the reserved register's num and offset from fcall's dst register
  // (shoud be r125.0)
  vASSERT(fcall->getDst()->isGreg());
  uint32_t reg_num =
      fcall->getDst()->getLinearizedStart() / kernel.numEltPerGRF<Type_UB>();
  uint32_t reg_off = fcall->getDst()->getLinearizedStart() %
                     kernel.numEltPerGRF<Type_UB>() /
                     fcall->getDst()->getTypeSize();

  G4_Declare *new_target_decl =
      createInstsForCallTargetOffset(insts, fcall, -64);

  // add  r125.0   IP   32
  G4_Declare *ret_decl = builder.createHardwiredDeclare(
      1, fcall->getDst()->getType(), reg_num, reg_off);
  insts.push_back(builder.createBinOp(
      G4_add, g4::SIMD1, builder.createDstRegRegion(ret_decl, 1),
      builder.createSrc(builder.phyregpool.getIpReg(), 0, 0,
                        builder.getRegionScalar(), Type_UD),
      builder.createImm(32, Type_UD), InstOpt_WriteEnable | InstOpt_NoCompact,
      false));

  // jmpi r2.0
  // update jump target (src0) to add's dst
  G4_SrcRegRegion *jump_target =
      builder.createSrcRegRegion(new_target_decl, builder.getRegionScalar());
  jump_target->setType(builder, Type_D);
  insts.push_back(
      builder.createJmp(nullptr, jump_target, InstOpt_NoCompact, false));
}

void Optimizer::expandIndirectCallWithRegTarget() {
  if (builder.hasFusedEU() && builder.getuint32Option(vISA_fusedCallWA) == 1) {
    vASSERT(!builder.needReplaceIndirectCallWithJmpi());
    // Relative IP has been applied in fusedCallWA()
    return;
  }

  // check every fcall
  for (auto bb : kernel.fg) {
    if (bb->back()->isFCall()) {
      G4_InstCF *fcall = bb->back()->asCFInst();
      if (fcall->isIndirectCall()) {
        // at this point the call instruction's src0 has the target_address
        // and the call dst is the reserved register (r125.0) for ret
        // All the caller save register should be saved. We usd r2 directly
        // here to calculate the new call's target.
        //
        // expand call
        // From:
        //       call r125.0 call_target
        // To:
        //       add  r2.0  -IP   call_target
        //       add  r2.0  r2.0  -32
        //       call r125.0  r2.0

        // For SKL workaround, expand call
        // From:
        //       call r125.0 call_target
        // To:
        //       add  r2.0     -IP    call_target
        //       add  r2.0     r2.0   -64
        //       add  r125.0   IP     32          // set the return IP
        //       jmpi r2.0
        InstListType expanded_insts;
        if (builder.needReplaceIndirectCallWithJmpi()) {
          createInstForJmpiSequence(expanded_insts, fcall);
        } else {
          G4_Declare *jmp_target_decl =
              createInstsForCallTargetOffset(expanded_insts, fcall, -32);
          // Updated call's target to the new target
          G4_SrcRegRegion *jump_target = builder.createSrcRegRegion(
              jmp_target_decl, builder.getRegionScalar());
          fcall->setSrc(jump_target, 0);
          fcall->setNoCompacted();
        }
        // then insert the expaneded instructions right before the call
        INST_LIST_ITER insert_point = bb->end();
        --insert_point;
        for (auto inst_to_add : expanded_insts) {
          bb->insertBefore(insert_point, inst_to_add);
        }

        // remove call from the instlist for Jmpi WA
        if (builder.needReplaceIndirectCallWithJmpi())
          bb->erase(--bb->end());
      }
    }
  }
}

// Replace ret with jmpi, must be single return
void Optimizer::replaceRetWithJmpi() {
  size_t num_ret = 0;

  for (G4_BB *bb : kernel.fg) {
    if (bb->empty())
      continue;
    if (bb->isEndWithFRet()) {
      ++num_ret;
      G4_INST *ret_inst = bb->back();

      // ret dst's decl
      G4_Declare *ret_reg = ret_inst->getSrc(0)->getTopDcl();

      // calculate the jmpi target offset
      // expand the original ret from:
      //     ret r125.0
      // To:
      //     add   r125.0  -ip   r125.0
      //     add   r125.0  r125.0  -48
      //     jmpi  r125.0

      // add   r125.0  -ip   r125.0
      G4_INST *add0 = builder.createBinOp(
          G4_add, g4::SIMD1, builder.createDstRegRegion(ret_reg, 1),
          builder.createSrcRegRegion(Mod_Minus, Direct,
                                     builder.phyregpool.getIpReg(), 0, 0,
                                     builder.getRegionScalar(), Type_UD),
          builder.createSrcRegRegion(ret_reg, builder.getRegionScalar()),
          InstOpt_WriteEnable | InstOpt_NoCompact, false);

      // add   r125.0  r125.0  -48
      G4_INST *add1 = builder.createBinOp(
          G4_add, g4::SIMD1, builder.createDstRegRegion(ret_reg, 1),
          builder.createSrcRegRegion(ret_reg, builder.getRegionScalar()),
          builder.createImm(-48, Type_D),
          InstOpt_WriteEnable | InstOpt_NoCompact, false);

      // jmpi r125.0
      G4_SrcRegRegion *jmpi_target =
          builder.createSrcRegRegion(ret_reg, builder.getRegionScalar());
      jmpi_target->setType(builder, Type_D);
      G4_INST *jmpi =
          builder.createJmp(nullptr, jmpi_target, InstOpt_NoCompact, false);

      // remove the ret
      bb->pop_back();
      // add the jmpi
      bb->push_back(add0);
      bb->push_back(add1);
      bb->push_back(jmpi);
    }
  }

  // there should be exactly one ret in a external function. We did not try
  // to restore the CallMask. We rely on single return of a function to make
  // sure the CallMask before and after calling this function is the same.
  vASSERT(num_ret == 1);
}

// Set a0 to tdr0 before sendc/sendsc
void Optimizer::setA0toTdrForSendc() {
  // check for the last inst of each BB, if it's sendc/sendsc, insert
  // "(W) mov(8) a0.0:uw tdr0.0:uw" right before it
  for (G4_BB *bb : kernel.fg) {
    if (bb->empty())
      continue;
    if (bb->back()->isSendConditional()) {
      // "(W) mov(8) a0.0:uw tdr0.0:uw"
      bb->insertBefore(
          --bb->end(),
          builder.createMov(g4::SIMD8,
                            builder.createDst(builder.phyregpool.getAddrReg(),
                                              0, 0, 1, Type_UW),
                            builder.createSrc(builder.phyregpool.getTDRReg(), 0,
                                              0, builder.getRegionScalar(),
                                              Type_UW),
                            InstOpt_WriteEnable, false));
    }
  }
}

// Check if there is WAR/WAW dependency between end inst and the preceding
// instruction
bool Optimizer::chkFwdOutputHazard(INST_LIST_ITER &startIter,
                                   INST_LIST_ITER &endIter) {
  G4_INST *startInst = *startIter;

  INST_LIST_ITER forwardIter = startIter;
  forwardIter++;
  while (forwardIter != endIter) {
    if ((*forwardIter)->isWAWdep(startInst) ||
        (*forwardIter)->isWARdep(startInst)) {
      break;
    }
    forwardIter++;
  }

  if (forwardIter != endIter) {
    return true;
  } else {
    return false;
  }
}

// check if startInst has any WAR/WAW conflicts with subsequent insts up till
// endIter (excluding endIter) precondition: startInst must be before endIter in
// the same BB this is used to sink an inst (or its sources) to the endIter
// location
bool Optimizer::chkFwdOutputHazard(G4_INST *startInst, INST_LIST_ITER endIter) {
  INST_LIST_ITER backIter = std::prev(endIter, 1);
  while (*backIter != startInst) {
    G4_INST *inst = *backIter;
    if (inst->isWARdep(startInst) || inst->isWAWdep(startInst)) {
      return true;
    }
    --backIter;
  }
  return false;
}

bool Optimizer::chkBwdOutputHazard(INST_LIST_ITER &startIter,
                                   INST_LIST_ITER &endIter) {
  G4_INST *endInst = *endIter;

  INST_LIST_ITER backwardIter = endIter;
  backwardIter--;
  while (backwardIter != startIter) {
    if (endInst->isWAWdep(*backwardIter) || endInst->isWARdep(*backwardIter)) {
      break;
    }
    backwardIter--;
  }

  if (backwardIter != startIter) {
    return true;
  } else {
    return false;
  }
}

bool Optimizer::chkBwdOutputHazard(G4_INST *startInst,
                                   INST_LIST_ITER &endIter) {
  G4_INST *endInst = *endIter;

  INST_LIST_ITER backwardIter = endIter;
  backwardIter--;
  while (*backwardIter != startInst) {
    if (endInst->isWAWdep(*backwardIter) ||
        //    Makes sure there is not WAR conflict between this instruction and
        //    instruction preceding it:
        //    ... grf1(use preceding inst)
        //    grf1 <---- def this inst
        endInst->isWARdep(*backwardIter)) {
      break;
    }
    backwardIter--;
  }

  if (*backwardIter != startInst) {
    return true;
  } else {
    return false;
  }
}

/*
Skips WAW check for the skipInst
*/
bool Optimizer::chkBwdOutputHazard(G4_INST *startInst, INST_LIST_ITER &endIter,
                                   G4_INST *skipInst) {
  G4_INST *endInst = *endIter;

  INST_LIST_ITER backwardIter = endIter;
  backwardIter--;
  while (*backwardIter != startInst) {
    if (skipInst == *backwardIter) {
      --backwardIter;
      continue;
    }

    // Makes sure there is not WAR conflict between this instruction and
    // instruction preceding it:
    // ... grf1(use preceding inst)
    // grf1 <---- def this inst
    if (endInst->isWARdep(*backwardIter) || endInst->isWAWdep(*backwardIter)) {
      break;
    }
    --backwardIter;
  }

  if (*backwardIter != startInst) {
    return true;
  } else {
    return false;
  }
}

bool Optimizer::chkBwdWARdep(G4_INST *startInst, INST_LIST_ITER endIter) {
  while (*endIter != startInst) {
    G4_INST *inst = *endIter;
    if (inst->isWARdep(startInst)) {
      return true;
    }
    --endIter;
  }
  return false;
}

// Check if there is WAW dependency between startInst and subsequent insts till
// endIter
bool Optimizer::chkBwdWAWdep(G4_INST *startInst, INST_LIST_ITER endIter) {
  INST_LIST_ITER backIter = std::prev(endIter, 1);
  while (*backIter != startInst) {
    G4_INST *inst = *backIter;
    if (inst->isWAWdep(startInst)) {
      return true;
    }
    --backIter;
  }
  return false;
}

// This function performs the following renaming to enable further optimization
// opportunities:
//
// op   v3, v1, v2
// mov  v4, v3
// mov  v5, v3
// mov  v6, v3
// ======>
// op   v3, v1, v2
// mov  v4, v3
// mov  v5, v4
// mov  v6, v4
void Optimizer::renameRegister() {
  const int MAX_REG_RENAME_DIST = 250;
  const int MAX_REG_RENAME_SIZE = 2;

  for (G4_BB *bb : fg) {
    bb->resetLocalIds();
    std::unordered_set<G4_INST *> Seen;

    INST_LIST_ITER ii = bb->begin(), iend(bb->end());
    while (ii != iend) {
      G4_INST *inst = *ii;

      if (!inst->isRawMov() || inst->getPredicate() || Seen.count(inst) > 0 ||
          inst->def_size() != 1 ||
          !inst->canHoist(!bb->isAllLaneActive(), fg.builder->getOptions())) {
        ii++;
        continue;
      }

      G4_Operand *src = inst->getSrc(0);
      G4_DstRegRegion *dst = inst->getDst();

      G4_Declare *srcDcl =
          src->isRegRegion() ? GetTopDclFromRegRegion(src) : nullptr;
      G4_Declare *dstDcl = GetTopDclFromRegRegion(dst);

      if (!srcDcl || !dstDcl) {
        ++ii;
        continue;
      }

      // If this move is between two different register files, then
      // do not do register renaming.
      if (srcDcl && dstDcl && srcDcl->getRegFile() != dstDcl->getRegFile()) {
        ++ii;
        continue;
      }

      G4_INST *defInst = inst->def_front().first;
      G4_Declare *defDstDcl = GetTopDclFromRegRegion(defInst->getDst());
      if ((dstDcl && dstDcl->getAddressed()) ||
          (defDstDcl && defDstDcl->getAddressed())) {
        ii++;
        continue;
      }

      G4_DstRegRegion *defDstRegion = defInst->getDst();
      if (Seen.count(defInst) > 0 ||
          src->compareOperand(defDstRegion, builder) != Rel_eq) {
        ii++;
        continue;
      }

      unsigned int instMaskOption = inst->getMaskOption();
      bool canRename = true;

      if (defInst->use_size() == 1) {
        ii++;
        continue;
      }

      int32_t sizeRatio = dstDcl->getByteSize() / srcDcl->getByteSize();

      G4_INST *lastUse = defInst->use_front().first;
      for (auto iter = defInst->use_begin(), E = defInst->use_end(); iter != E;
           ++iter) {
        G4_INST *useInst = (*iter).first;

        if (useInst == inst || ((useInst->getLocalId() - inst->getLocalId()) >
                                    MAX_REG_RENAME_DIST &&
                                sizeRatio > MAX_REG_RENAME_SIZE)) {
          continue;
        }

        /*
            it incorrectly renames in this case, because it doesn't consider
            defInst mask.
            BEFORE:
            <TR><TD ALIGN="LEFT"><FONT color="black">0:mov (1) V44(0,0)[1]:d
           V46(0,0)[0;1,0]:d [Align1, NoMask] %11</FONT></TD></TR> <TR><TD
           ALIGN="LEFT"><FONT color="black">0:mov (8) V48(0,0)[1]:d
           V44(0,0)[0;1,0]:d [Align1, Q1] %12</FONT></TD></TR> <TR><TD
           ALIGN="LEFT"><FONT color="black">0:mov (8) V56(0,0)[1]:d
           V44(0,0)[0;1,0]:d [Align1, Q1] %22</FONT></TD></TR>

            AFTER:
            <TR><TD ALIGN="LEFT"><FONT color="black">0:mov (1) V44(0,0)[1]:d
           V46(0,0)[0;1,0]:d [Align1, NoMask] %11</FONT></TD></TR> <TR><TD
           ALIGN="LEFT"><FONT color="black">0:mov (8) V48(0,0)[1]:d
           V44(0,0)[0;1,0]:d [Align1, Q1] %12</FONT></TD></TR> <TR><TD
           ALIGN="LEFT"><FONT color="black">0:mov (8) V56(0,0)[1]:d
           V48(0,0)[0;1,0]:d [Align1, Q1] %22</FONT></TD></TR>

            Fix: BB in SIMD control flow && (inst not NoMask) !(instMask ==
           defFask == useMask)

            Disallow replication?
        */
        if (useInst->getLocalId() < inst->getLocalId() ||
            !useInst->isRawMov() ||
            inst->getExecSize() != useInst->getExecSize() ||
            (useInst->getSrc(0))->compareOperand(defDstRegion, builder) !=
                Rel_eq ||
            useInst->def_size() > 1 ||
            (!(inst->isWriteEnableInst()) &&
             useInst->getMaskOption() != instMaskOption) ||
            // fix described above
            (!bb->isAllLaneActive() && !inst->isWriteEnableInst() &&
             !(inst->getExecSize() == defInst->getExecSize() &&
               inst->getExecSize() == useInst->getExecSize()))) {
          canRename = false;
          break;
        }

        if (useInst->getLocalId() > lastUse->getLocalId()) {
          lastUse = useInst;
        }
      }

      for (auto iter = defInst->use_begin(), E = defInst->use_end(); iter != E;
           ++iter) {
        G4_INST *useInst = (*iter).first;
        Seen.insert(useInst);
      }

      if (!canRename) {
        ii++;
        continue;
      }

      INST_LIST_ITER forwardIter = ii;
      forwardIter++;
      while (canRename && *forwardIter != lastUse &&
             (((*forwardIter)->getLocalId() - inst->getLocalId()) <=
                  MAX_REG_RENAME_DIST ||
              sizeRatio <= MAX_REG_RENAME_SIZE)) {
        if ((*forwardIter)->isWAWdep(inst)) {
          canRename = false;
          break;
        }
        forwardIter++;
      }

      if (!canRename) {
        ii++;
        continue;
      }

      for (auto useIter = defInst->use_begin(); useIter != defInst->use_end();
           /*empty*/) {
        G4_INST *useInst = (*useIter).first;

        if (useInst == inst || ((useInst->getLocalId() - inst->getLocalId()) >
                                    MAX_REG_RENAME_DIST &&
                                sizeRatio > MAX_REG_RENAME_SIZE)) {
          useIter++;
          continue;
        }

        G4_Operand *useSrc = useInst->getSrc(0);
        unsigned char execSize = useInst->getExecSize();
        unsigned short dstHS = dst->getHorzStride();
        const RegionDesc *newSrcRd;

        if (useSrc->asSrcRegRegion()->isScalar()) {
          newSrcRd = builder.getRegionScalar();
        } else {
          unsigned tExecSize = (execSize > 8) ? 8 : execSize;
          if (RegionDesc::isLegal(tExecSize * dstHS, execSize, dstHS) &&
              (execSize * dstHS <= 32)) { // VS at most 32
            newSrcRd = builder.createRegionDesc((uint16_t)tExecSize * dstHS,
                                                execSize, dstHS);
          } else {
            // Skip this use. TODO: normalize this region.
            ++useIter;
            continue;
          }
        }

        G4_SrcRegRegion *newSrcOpnd = builder.createSrcRegRegion(
            Mod_src_undef, dst->getRegAccess(), dst->getBase(),
            dst->getRegOff(), dst->getSubRegOff(), newSrcRd, useSrc->getType());
        if (dst->getRegAccess() != Direct) {
          newSrcOpnd->asSrcRegRegion()->setImmAddrOff(dst->getAddrImm());
        }
        useInst->setSrc(newSrcOpnd, 0);

        // Maintain def-use for this change:
        // - remove this use from defInst
        // - add a new use to inst
        useIter = defInst->eraseUse(useIter);
        inst->addDefUse(useInst, Opnd_src0);
      }

      ii++;
    }
  }
}

//
// recompute bounds for declares in the given unordered_set
// this only affects DstRegRegion and SrcRegRegion
// If the operand is global, we have to update the global operand table as well
// as the bounds may have changed
//
void Optimizer::recomputeBound(std::unordered_set<G4_Declare *> &declares) {

  for (auto bb : fg) {
    for (auto ii = bb->begin(), iiEnd = bb->end(); ii != iiEnd; ++ii) {
      G4_INST *inst = *ii;
      if (inst->getDst() != NULL) {
        G4_DstRegRegion *dst = inst->getDst();
        if (dst->getTopDcl() != NULL &&
            declares.find(dst->getTopDcl()) != declares.end()) {
          bool isGlobal = builder.kernel.fg.globalOpndHT.isOpndGlobal(dst);
          dst->computeLeftBound(builder);
          inst->computeRightBound(dst);
          if (isGlobal) {
            builder.kernel.fg.globalOpndHT.addGlobalOpnd(dst);
          }
        }
      }
      for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
        if (inst->getSrc(i) != NULL && inst->getSrc(i)->isSrcRegRegion()) {
          G4_SrcRegRegion *src = inst->getSrc(i)->asSrcRegRegion();
          if (src->getTopDcl() != NULL &&
              declares.find(src->getTopDcl()) != declares.end()) {
            bool isGlobal = builder.kernel.fg.globalOpndHT.isOpndGlobal(src);
            src->computeLeftBound(builder);
            inst->computeRightBound(src);
            if (isGlobal) {
              builder.kernel.fg.globalOpndHT.addGlobalOpnd(src);
            }
          }
        }
      }
    }
  }
}

//
// Given a sequence of simd1 instructions (max 4), try to merge them into a
// single instruction e.g., mul (1) r0.4<1>:f r0.0<0;1,0>:f r6.5<0;1,0>:f
// {NoMask} mul (1) r0.5<1>:f r0.1<0;1,0>:f r6.5<0;1,0>:f {NoMask} mul (1)
// r0.6<1>:f r0.2<0;1,0>:f r6.5<0;1,0>:f {NoMask} mul (1) r0.7<1>:f
// r0.3<0;1,0>:f r6.5<0;1,0>:f {NoMask} becomes mul (4) r0.4<1>:f r0.0<1;1,0>:f
// r6.5<0;1,0>:f {NoMask} A bunch of conditions have to be satisified; check
// BUNDLE_INFO for more details. This is only performed for 3D input as CM is
// very unlikely to benefit from this (put another way, if this succeeds for CM
// our FE is doing something wrong)
//
void Optimizer::mergeScalarInst() {

  int bundleSizeLimit = BUNDLE_INFO::maxBundleSize;
  if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D || builder.noInt64()) {
    bundleSizeLimit = 4;
  }

  // set of declares that have been changed to alias to another declare
  std::unordered_set<G4_Declare *> modifiedDcl;
  std::vector<G4_Declare *> newInputs;

  // stats
  int numBundles = 0;
  int numDeletedInst = 0;

  for (G4_BB *bb : fg) {
    std::vector<BUNDLE_INFO> bundles;
    INST_LIST_ITER ii = bb->begin(), iiEnd = bb->end();
    while (ii != iiEnd) {
      G4_INST *inst = *ii;
      auto nextIter = ii;
      ++nextIter;
      if (nextIter != iiEnd && BUNDLE_INFO::isMergeCandidate(
                                   inst, builder, !bb->isAllLaneActive())) {
        BUNDLE_INFO bundle(bb, ii, bundleSizeLimit);
        bundle.findInstructionToMerge(nextIter, builder);
        if (bundle.size > 1)
          bundles.emplace_back(bundle);
        ii = nextIter;
      } else {
        ++ii;
      }
    }

    for (auto &bundle : bundles) {
      bool success = bundle.doMerge(builder, modifiedDcl, newInputs);
      if (success) {
        numBundles++;
        numDeletedInst += bundle.size - 1;
      }
    }
  }

  // we have to reset the bound for all operands whose declares have been
  // modified
  recomputeBound(modifiedDcl);

  VISA_DEBUG({
    std::cout << "             === Merge Scalar Optimization ===\n";
    std::cout << "Number of optimized bundles:\t" << numBundles << "\n";
    std::cout << "Number of instructions saved:\t" << numDeletedInst << "\n";
  });
}

static bool isMad(G4_INST *I) {
  // Disable int mac for PVC
  auto dst = I->getDst();
  return (I->opcode() == G4_pseudo_mad &&
          !(I->getBuilder().waDisableIntMac() && dst &&
            I->isIntegerPipeType(dst->getType())));
}

static inline bool isWBTypeAndNotNull(G4_Operand *opnd) {
  // Requires opnd to be not null.
  return opnd && (IS_BTYPE(opnd->getType()) || IS_WTYPE(opnd->getType()));
}

namespace {

/// Class to describe a mad sequence that can be turned into a sequence of MAC
/// instructions.
class MadSequenceInfo {
public:
  // IR builder object.
  IR_Builder &builder;

  // The basic block being examined.
  G4_BB *bb;

  enum MadSeqKind { MK_unknown, MK_isSafe, MK_isNotSafe };

private:
  // Flag indicates if ACC transformation is safe.
  MadSeqKind kind;

  // The single definition that defines the first mad's src2. If there are
  // multiple defintions, it is nullptr.
  G4_INST *src2Def;

  // The sequence of mad instruction to be examined.
  std::vector<G4_INST *> madSequence;

  // The use chain of the last mad. This use chain ended with an instruction
  // that has *B/*W type, and the chain length is limited by a predefined
  // constant.
  std::vector<G4_INST *> lastMadUserChain;

public:
  MadSequenceInfo(IR_Builder &builder, G4_BB *bb)
      : builder(builder), bb(bb), kind(MK_unknown), src2Def(nullptr) {}

  bool isSafe() const { return kind == MK_isSafe; }
  bool isNotSafe() const { return kind == MK_isNotSafe; }
  void setNotSafe() { kind = MK_isNotSafe; }
  void setSafe() { kind = MK_isSafe; }

  G4_INST *getSrc2Def() { return src2Def; }
  G4_INST *getFirstMad() const { return madSequence.front(); }
  G4_INST *getLastMad() const { return madSequence.back(); }
  void appendMad(INST_LIST_ITER begin, INST_LIST_ITER end) {
    madSequence.insert(madSequence.end(), begin, end);
  }
  typedef std::vector<G4_INST *>::iterator mad_iter;
  mad_iter mad_begin() { return madSequence.begin(); }
  mad_iter mad_end() { return madSequence.end(); }

  void appendUser(G4_INST *inst) { lastMadUserChain.push_back(inst); }
  G4_INST *getLastUser() { return lastMadUserChain.back(); }

  void reset() {
    kind = MK_unknown;
    src2Def = nullptr;
    madSequence.clear();
    lastMadUserChain.clear();
  }

  // Collect all candidate instructions and perform minimal checks.
  INST_LIST_ITER populateCandidates(INST_LIST_ITER iter);

  // Make changes to all the candidates collected. This is the only
  // function that makes changes to the IR.
  void processCandidates();

private:
  void populateUserChain(G4_INST *defInst, int level);
  void populateSrc2Def();

  // Check whether this mad sequence can be turned into a MAC sequence.
  bool checkMadSequence();

  // Check whether the user chain blocks this transformation or not.
  bool checkUserChain();

  // Check if other instructions between defInst and useInst are also updating
  // ACC registers, which may block this transformation.
  bool checkACCDependency(G4_INST *defInst, G4_INST *useInst);

  // The common type for accumulator operands.
  G4_Type getCommonACCType() {
    G4_Type T = Type_UNDEF;

    // If there is no user chain to check. Use the last mad's destionation
    // operand type as the common type. Otherwise, use the last user's
    // destionation type.
    if (lastMadUserChain.empty())
      T = getLastMad()->getDst()->getType();
    else
      T = getLastUser()->getDst()->getType();

    vISA_ASSERT((IS_FTYPE(T) || IS_HFTYPE(T) || IS_INT(T)),
                "Only F/HF/W/B types are expected here");
    return (IS_FTYPE(T) || IS_HFTYPE(T))
               ? T
               : (IS_SIGNED_INT(T) ? Type_W : Type_UW);
  }
};

class AccRestriction {
  unsigned encoding;

public:
  // Accumulator Restriction Kind:
  enum Accumulator_RK {
    ARK_NoRestriction = 0x01,   // No restrictions.
    ARK_NoAccess = 0x02,        // No accumulator access, implicit or explicit.
    ARK_NoSourceOperand = 0x04, // Source operands cannot be accumulators.
    ARK_NoModifier =
        0x08, // Source modifier is not allowed if source is an accumulator.
    ARK_NoExplicitSource = 0x010, // Accumulator is an implicit source and thus
                                  // cannot be an explicit source operand.
    ARK_NoDst =
        0x20, // Accumulator cannot be destination, implicit or explicit.
    ARK_AccWrEnRequired =
        0x40, // AccWrEn is required. The accumulator is an implicit
              // destination and thus cannot be an explicit destination operand.
    ARK_NoIntegerSource =
        0x80, // Integer source operands cannot be accumulators.
    ARK_NoExplicitSrcAllowAccWrEn =
        0x100, // No explicit accumulator access because this is a three-source
               // instruction. AccWrEn is allowed for implicitly updating the
               // accumulator.
    ARK_NoBothSrcAndDst = 0x200 // An accumulator can be a source or destination
                                // operand but not both.
  };

  AccRestriction(unsigned val) : encoding(val) {}

  bool useAccAsSrc(G4_SrcRegRegion *opnd, bool isExplicit = true,
                   bool isAlreadyDst = false) const {
    if (!opnd)
      return false;

    if (encoding & ARK_NoAccess)
      return false;

    if (encoding & ARK_NoRestriction)
      return true;

    if (encoding & ARK_NoSourceOperand)
      return false;

    if (encoding & ARK_NoIntegerSource)
      return !IS_TYPE_INT(opnd->getType());

    if (encoding & ARK_NoExplicitSource)
      return !isExplicit;

    if (encoding & ARK_NoExplicitSrcAllowAccWrEn)
      return false;

    if (encoding & ARK_NoBothSrcAndDst)
      return !isAlreadyDst;

    if (encoding & ARK_NoModifier)
      return opnd->getModifier() == Mod_src_undef;

    return true;
  }

  bool useAccAsDst(G4_DstRegRegion *opnd, bool isExplicit = true,
                   bool isAlreadySrc = false) const {
    if (!opnd)
      return false;

    if (encoding & ARK_NoAccess)
      return false;

    if (encoding & ARK_NoRestriction)
      return true;

    if (encoding & ARK_NoDst)
      return false;

    if (encoding & ARK_AccWrEnRequired)
      return !isExplicit;

    if (encoding & ARK_NoBothSrcAndDst)
      return !isAlreadySrc;

    return true;
  }

  static AccRestriction getRestrictionKind(G4_INST *inst);
};

} // namespace

AccRestriction AccRestriction::getRestrictionKind(G4_INST *inst) {
  switch (inst->opcode()) {
  default:
    break;
  case G4_add:
  case G4_asr:
  case G4_avg:
  case G4_csel:
  case G4_frc:
  case G4_sel:
  case G4_shr:
  case G4_smov:
    return ARK_NoRestriction;
  case G4_addc:
  case G4_subb:
    return ARK_AccWrEnRequired;
  case G4_and:
  case G4_not:
  case G4_or:
  case G4_xor:
    return ARK_NoModifier;
  case G4_cmp:
  case G4_cmpn:
  case G4_lzd:
    return ARK_NoRestriction;
  case G4_dp2:
  case G4_dp3:
  case G4_dp4:
  case G4_dph:
  case G4_line:
  case G4_movi:
  case G4_pln:
  case G4_sad2:
  case G4_sada2:
    return ARK_NoSourceOperand;
  case G4_lrp:
  case G4_mac:
    return ARK_NoExplicitSource;
  case G4_madm:
    return ARK_NoExplicitSrcAllowAccWrEn;
  case G4_mach:
    return ARK_NoExplicitSource | ARK_AccWrEnRequired;
  case G4_mov:
    return ARK_NoBothSrcAndDst;
  case G4_mul:
    return ARK_NoIntegerSource;
  case G4_rndd:
  case G4_rnde:
  case G4_rndu:
  case G4_rndz:
    return ARK_NoRestriction;
  case G4_shl:
    return ARK_NoDst;
  }

  return ARK_NoAccess;
}

/// Check this pseudo-mad's dst operand. Returns false if there is anything
/// blocking acc's usage.
static bool checkMadDst(G4_INST *inst, IR_Builder &builder) {
  G4_DstRegRegion *dst = inst->getDst();
  if (!dst || builder.kernel.fg.globalOpndHT.isOpndGlobal(dst))
    return false;

  if (dst->getRegAccess() != Direct)
    return false;

  // Only acc0 is available for w/uw destination.
  // FIXME: This acc type size is only for simd 16.
  unsigned Sz = TypeSize(Type_W);
  Sz *= dst->getHorzStride() * inst->getExecSize();
  return Sz <= builder.numEltPerGRF<Type_UB>();
}

// Check whether this mad sequence can be turned into a MAC sequence.
bool MadSequenceInfo::checkMadSequence() {
  unsigned int maskOffset = getFirstMad()->getMaskOffset();

  // First check each individual mad.
  for (auto inst : madSequence) {
    vISA_ASSERT(isMad(inst), "not a mad");

    // Only for simd 16. TODO: support simd8.
    if (inst->getExecSize() != g4::SIMD16)
      return false;

    if (inst->getMaskOffset() != maskOffset)
      return false;

    // Do not handle predicate yet.
    if (inst->getPredicate() != nullptr)
      return false;

    // Do not handle cond modifier yet.
    if (inst->getCondMod() != nullptr)
      return false;

    if (!checkMadDst(inst, builder))
      return false;

    G4_Operand *src0 = inst->getSrc(0);
    G4_Operand *src1 = inst->getSrc(1);
    G4_Operand *src2 = inst->getSrc(2);

    if (!src0 || !src1 || !src2)
      return false;

    if (builder.noDFTypeMac()) {
      if (IS_DFTYPE(src0->getType()) || IS_DFTYPE(src1->getType()) ||
          IS_DFTYPE(src2->getType()) || IS_DFTYPE(inst->getDst()->getType()))
        return false;
    }

    if (IS_FTYPE(src0->getType()) && IS_FTYPE(src1->getType()) &&
        IS_FTYPE(src2->getType())) {
      // ok
    } else if ((!IS_BTYPE(src0->getType()) && !IS_WTYPE(src0->getType())) ||
               (!IS_BTYPE(src1->getType()) && !IS_WTYPE(src1->getType()))) {
      // Only when src0 and src1 are of Byte/Word types.
      return false;
    } else if (!IS_BTYPE(src2->getType()) && !IS_WTYPE(src2->getType()) &&
               !IS_DTYPE(src2->getType())) {
      // Only when src2 is of Byte/Word/DWord types.
      return false;
    }

    if (!builder.hasByteALU()) {
      if (IS_BTYPE(src0->getType()) || IS_BTYPE(src1->getType())) {
        return false;
      }
    }

    if (builder.avoidAccDstWithIndirectSource()) {
      if (src0->isSrcRegRegion() && src0->asSrcRegRegion()->isIndirect()) {
        return false;
      }
      if (src1->isSrcRegRegion() && src1->asSrcRegRegion()->isIndirect()) {
        return false;
      }
    }

    // If there is a modifier for src2, or src2 is accessed somewhere
    // indirectly then we will not generate a MAC.
    if (!src2->isSrcRegRegion())
      return false;

    if (src2->asSrcRegRegion()->getModifier() != Mod_src_undef ||
        src2->asSrcRegRegion()->getRegAccess() != Direct ||
        (src2->getTopDcl() && src2->getTopDcl()->getAddressed()))
      return false;
  }

  // Now check instructions in pairs.
  G4_INST *defInst = getSrc2Def();
  G4_INST *lastMad = getLastMad();
  for (auto I = mad_begin(); defInst != lastMad; ++I) {
    G4_INST *useInst = *I;
    G4_Operand *dst = defInst->getDst();
    G4_Operand *src2 = useInst->getSrc(2);
    vISA_ASSERT(dst && dst->isDstRegRegion(), "invalid dst");
    vISA_ASSERT(src2 && src2->isSrcRegRegion(), "invalid src2");
    if (dst->compareOperand(src2, builder) != Rel_eq)
      return false;

    // Move the next pair.
    defInst = useInst;
  }

  return true;
}

bool MadSequenceInfo::checkACCDependency(G4_INST *defInst, G4_INST *useInst) {
  auto iter = std::find(bb->begin(), bb->end(), defInst);
  vISA_ASSERT(iter != bb->end(), "no instruction found?");

  for (++iter; (*iter) != useInst; ++iter) {
    if ((*iter)->defAcc() || (*iter)->useAcc() ||
        (*iter)->mayExpandToAccMacro())
      return false;
  }
  return true;
}

// Check whether this user chain is safe to use ACC.
bool MadSequenceInfo::checkUserChain() {
  // skip if there is no user to be analyzed.
  if (lastMadUserChain.empty())
    return true;

  G4_INST *defInst = getLastMad();
  G4_INST *useInst = defInst->use_back().first;

  while (true) {
    // TODO: enable simd8.
    if (useInst->getExecSize() != g4::SIMD16)
      return false;

    // Only when used as a source operand.
    Gen4_Operand_Number opndNum = defInst->use_back().second;
    if (!G4_INST::isSrcNum(opndNum))
      return false;

    G4_Operand *useOpnd = useInst->getSrc(G4_INST::getSrcNum(opndNum));
    if (!useOpnd || !useOpnd->isSrcRegRegion())
      return false;

    if (useOpnd->asSrcRegRegion()->isIndirect())
      return false;

    // check other source type.
    for (int i = 0, e = useInst->getNumSrc(); i < e; ++i) {
      G4_Operand *otherSrc = useInst->getSrc(i);
      if (otherSrc == useOpnd)
        continue;

      if (!isWBTypeAndNotNull(otherSrc))
        return false;
    }

    bool isLastUser = (useInst == getLastUser());

    // The last user does not use ACC as its dst.
    AccRestriction AR = AccRestriction::getRestrictionKind(useInst);
    if (!AR.useAccAsSrc(useOpnd->asSrcRegRegion(), true, !isLastUser))
      return false;

    // Now check between defInst and useInst, no ACC will be written by
    // other instructions.
    if (!checkACCDependency(defInst, useInst))
      return false;

    if (isLastUser)
      // No extra check for the last user.
      break;

    // This is not the last user. We check its dst too.
    G4_Operand *useDst = useInst->getDst();
    if (!useDst)
      return false;

    // check type, no support for *Q types yet.
    if (!IS_DTYPE(useDst->getType()))
      return false;

    // For each inner user, need to use ACC as its explicit dst.
    if (!AR.useAccAsDst(useDst->asDstRegRegion(), true, true))
      return false;

    if (defInst->getDst()->compareOperand(useOpnd, builder) != Rel_eq)
      return false;

    // move to next pair.
    defInst = useInst;
    useInst = defInst->use_back().first;
  }

  // nothing wrong.
  return true;
}

void MadSequenceInfo::populateSrc2Def() {
  G4_INST *firstMad = getFirstMad();
  vISA_ASSERT(firstMad && isMad(firstMad), "invalid mad");

  src2Def = firstMad->getSingleDef(Opnd_src2, true);
  if (src2Def == nullptr)
    // Cannot find a single definition.
    return setNotSafe();

  // Check it right here.
  // Only support splats or simd16 initialization.
  if (src2Def->getExecSize() != g4::SIMD16 &&
      src2Def->getExecSize() != g4::SIMD1) {
    return setNotSafe();
  }

  G4_Operand *Dst = src2Def->getDst();
  if (!Dst || builder.kernel.fg.globalOpndHT.isOpndGlobal(Dst))
    return setNotSafe();

  if (Dst->asDstRegRegion()->getRegAccess() != Direct)
    return setNotSafe();

  if (src2Def->getPredicate() || src2Def->getSaturate() ||
      !src2Def->hasOneUse())
    return setNotSafe();

  if (!src2Def->canDstBeAcc())
    return setNotSafe();

  if (IS_DTYPE(src2Def->getExecType())) {
    // since we use <1>:w region for our acc temp, due to alignment requirements
    // we can't allow dword source types
    return setNotSafe();
  }

  if (!builder.hasByteALU()) {
    // do not allow acc if src2Dest inst has byte source
    for (int i = 0; i < src2Def->getNumSrc(); ++i) {
      if (IS_BTYPE(src2Def->getSrc(i)->getType())) {
        return setNotSafe();
      }
    }
  }

  if (builder.avoidAccDstWithIndirectSource()) {
    for (int i = 0; i < src2Def->getNumSrc(); ++i) {
      if (src2Def->getSrc(i)->isSrcRegRegion() &&
          src2Def->getSrc(i)->asSrcRegRegion()->isIndirect()) {
        return setNotSafe();
      }
    }
  }

  // Check if there is any ACC dependency.
  if (!checkACCDependency(src2Def, firstMad))
    return setNotSafe();

  // Check restrictions on compression to ensure that changing the destination
  // type will not change the source region meaning due to instruction
  // compression.
  //
  // If both instructions are compressed or both non-compressed then it is
  // safe. Otherwise, check whether source regions are compression invariants.
  if (src2Def->isComprInst() ^ firstMad->isComprInst()) {
    auto checkCompression = [](G4_INST *inst) {
      for (int i = 0; i < inst->getNumSrc(); ++i) {
        G4_Operand *opnd = inst->getSrc(i);
        if (!opnd || !opnd->isSrcRegRegion())
          continue;
        if (!inst->isComprInvariantSrcRegion(opnd->asSrcRegRegion(), i))
          return false;
        if (IS_DTYPE(opnd->getType()))
          return false;
      }
      return true;
    };

    if (src2Def->isComprInst()) {
      if (!checkCompression(src2Def))
        return setNotSafe();
    } else {
      if (!checkCompression(firstMad))
        return setNotSafe();
    }
  }

  AccRestriction AR = AccRestriction::getRestrictionKind(src2Def);
  if (!AR.useAccAsDst(Dst->asDstRegRegion()))
    return setNotSafe();
}

void MadSequenceInfo::populateUserChain(G4_INST *defInst, int level) {
  // Failed.
  if (level <= 0)
    return setNotSafe();

  // Only for a single use.
  if (!defInst->hasOneUse())
    return setNotSafe();

  // Only when used as a source operand.
  Gen4_Operand_Number opndNum = defInst->use_back().second;
  if (!G4_INST::isSrcNum(opndNum))
    return setNotSafe();

  G4_INST *useInst = defInst->use_back().first;
  auto useDst = useInst->getDst();

  if (useDst == nullptr)
    return setNotSafe();

  appendUser(useInst);

  // If this user has *W/*B types then candidate found and stop the search.
  if (IS_BTYPE(useDst->getType()) || IS_WTYPE(useDst->getType()))
    return;

  // Search to the next level.
  return populateUserChain(useInst, level - 1);
}

// Currently, we assume that MAD instructions are back-to-back. This avoids
// dependency checking among mad and non-mad instructions.
//
// TODO: hoist or sink instructions.
//
INST_LIST_ITER MadSequenceInfo::populateCandidates(INST_LIST_ITER iter) {
  // Find the first pseudo-mad instruction.
  iter = std::find_if(iter, bb->end(), isMad);

  // No mad found
  if (iter == bb->end())
    return iter;

  // Find the first non-mad instruction following this sequence.
  auto end = std::find_if_not(iter, bb->end(), isMad);

  vISA_ASSERT(iter != end, "out of sync");
  appendMad(iter, end);

  // Checking the first mad's src2.
  populateSrc2Def();

  // If the mad sequence has *W/*B types then it is safe to use ACC regardless
  // of the destionation operand type in its use.
  // ..
  // mac (16) acc0.0<1>:w r10.7<0;1,0>:w r62.16<16;16,1>:ub {Align1, H1}
  // mac (16) acc0.0<1>:w r11.6<0;1,0>:w r63.0<16;16,1>:ub {Align1, H1}
  // mac (16) r24.0<1>:w r11.7<0;1,0>:w r63.16<16;16,1>:ub {Align1, H1}
  // add (16) r14.0<1>:d r14.0<8;8,1>:d r24.0<8;8,1>:w {Align1, H1}
  //
  // could be generated.
  G4_Type MadDstType = getLastMad()->getDst()->getType();
  if (IS_DTYPE(MadDstType)) {
    // Populate the user chain up to some predetermined level.
    const int level = 4;
    populateUserChain(getLastMad(), level);
  }

  // We have gathered all candidates for this optimization. Now we make
  // comprehensive checks on the mad sequence and the user chain.
  if (isNotSafe())
    return end;

  if (!checkMadSequence())
    return end;

  if (!checkUserChain())
    return end;

  // everything is OK, preceed to do transformation.
  setSafe();
  return end;
}

// Makes changes to the mad sequence and its users.
void MadSequenceInfo::processCandidates() {
  vISA_ASSERT(isSafe(), "not safe for ACC");
  vISA_ASSERT(getSrc2Def(), "null src");
  vISA_ASSERT(!madSequence.empty(), "no mad");

  // In this function we replace all src2 to implicit ACC registers and
  // update operands in its use chain.
  G4_Type AdjustedType = getCommonACCType();

  // Fix src2Def
  G4_INST *src2Def = getSrc2Def();
  {
    // change dst of the last MAD
    G4_DstRegRegion *accDstOpnd = builder.createDst(
        builder.phyregpool.getAcc0Reg(), 0, 0, 1, AdjustedType);
    src2Def->setDest(accDstOpnd);

    // Convert splat.
    if (src2Def->getExecSize() == g4::SIMD1) {
      src2Def->setExecSize(getFirstMad()->getExecSize());
    }
  }

  // update use-chain
  if (!lastMadUserChain.empty()) {
    G4_INST *defInst = getLastMad();
    G4_INST *useInst = defInst->use_back().first;
    Gen4_Operand_Number opndNum = defInst->use_back().second;
    vISA_ASSERT(defInst->hasOneUse(), "bad candidate");

    while (true) {
      const RegionDesc *rd = builder.getRegionStride1();
      auto mod = useInst->getOperand(opndNum)->asSrcRegRegion()->getModifier();
      G4_SrcRegRegion *accSrcOpnd = builder.createSrcRegRegion(
          mod, Direct, builder.phyregpool.getAcc0Reg(), 0, 0, rd, AdjustedType);
      useInst->setSrc(accSrcOpnd, G4_INST::getSrcNum(opndNum));

      // The last use, only update source, and exit.
      if (useInst == getLastUser())
        break;

      // Also update the destination.
      G4_DstRegRegion *accDstOpnd = builder.createDst(
          builder.phyregpool.getAcc0Reg(), 0, 0, 1, AdjustedType);
      useInst->setDest(accDstOpnd);

      // move to the next pair.
      defInst = useInst;
      useInst = defInst->use_back().first;
      opndNum = defInst->use_back().second;
      vISA_ASSERT(defInst->hasOneUse(), "bad candidate");
    }
  }

  // update mad sequence
  for (auto I = mad_begin(), E = mad_end(); I != E; ++I) {
    G4_INST *inst = *I;
    vISA_ASSERT(isMad(inst), "not a mad");

    const RegionDesc *rd = builder.getRegionStride1();
    G4_SrcRegRegion *accSrcOpnd = builder.createSrc(
        builder.phyregpool.getAcc0Reg(), 0, 0, rd, AdjustedType);

    inst->setImplAccSrc(accSrcOpnd);
    inst->setSrc(nullptr, 2);
    // For the last mad, if it has *B/*W type, then no user will be modified
    // and do not change its destination operand. Otherwise, use acc as the
    // destination.
    if (getLastMad() != inst || !lastMadUserChain.empty()) {
      G4_DstRegRegion *accDstOpnd = builder.createDst(
          builder.phyregpool.getAcc0Reg(), 0, 0, 1, AdjustedType);
      inst->setDest(accDstOpnd);
    }
    inst->setOpcode(G4_mac);
    inst->fixMACSrc2DefUse();
  }
}

// Do any kind of proprocessing in this basic block to help MAC transformation.
// Returns false if we can easily detect this optimization is not possible.
// Otherwise, returns true.
static bool preprocessMadInBlock(IR_Builder &builder, G4_BB *bb) {
  bool hasMad = false;
  for (auto inst : *bb) {
    if (isMad(inst)) {
      hasMad = true;
      HWConformity::tryEliminateMadSrcModifier(builder, inst);
    }
  }

  // nothing to do if there is no mad.
  return hasMad;
}

// clang-format off
//
// mul (16) V48(0,0)<1>:d r0.1<0;1,0>:w V42_in(7,2)<16;16,1>:ub {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r1.0<0;1,0>:w V42_in(7,1)<16;16,1>:ub V48(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.2<0;1,0>:w V42_in(7,3)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.3<0;1,0>:w V42_in(8,1)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.4<0;1,0>:w V42_in(8,2)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.5<0;1,0>:w V42_in(8,3)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.6<0;1,0>:w V42_in(9,1)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.7<0;1,0>:w V42_in(9,2)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// psuedo_mad (16) V51_tempConvolve(0,0)<1>:d r0.8<0;1,0>:w V42_in(9,3)<16;16,1>:ub V51_tempConvolve(0,0)<16;16,1>:d {Align1, H1}
// add (16) V51_tempConvolve(0,0)<1>:d V51_tempConvolve(0,0)<16;16,1>:d 0x4000:w {Align1, H1}
// shr.sat (16) V52(0,0)<1>:ub V51_tempConvolve(0,0)<16;16,1>:d 0xf:w {Align1, H1}
//
// clang-format on
void Optimizer::lowerMadSequence() {

  // Only enable CM for now.
  if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) != VISA_CM)
    return;

  if (!builder.hasMacMacl())
    return;

  for (G4_BB *bb : fg) {
    // Preprocess this basic block. If no mad sequence found then skip to
    // the next basic block right away.
    if (!preprocessMadInBlock(builder, bb))
      continue;

    // Object to gather information for ACC optimization.
    MadSequenceInfo madInfo(builder, bb);

    auto iter = bb->begin();
    while (iter != bb->end()) {
      // Returns an iterator to the next non-mad instruction after the mad
      // sequence. It is safe to insert/delete instructions before it.
      iter = madInfo.populateCandidates(iter);

      // Perform transformation. The resulted IR may still need to be
      // fixed by HWConformity, e.g. the dst may still have *B type.
      if (madInfo.isSafe())
        madInfo.processCandidates();

      // Cleanup immediate results, whether change has been made or not.
      madInfo.reset();
    }
  }
}

void Optimizer::ifCvt() { runIfCvt(fg); }


namespace {

enum SplitBounds : unsigned {
  LoLBound = 0,
  LoRBound = 63,
  HiLBound = 64,
  HiRBound = 127
};

static bool isCandidateDecl(G4_Declare *Dcl, const IR_Builder &builder) {
  G4_Declare *RootDcl = Dcl->getRootDeclare();
  if (RootDcl->getRegFile() != G4_GRF)
    return false;

  // Only split 4GRF variables. We should be able to split > 4GRF variables,
  // but this should have been done in FE.
  if (RootDcl->getByteSize() != 4 * builder.numEltPerGRF<Type_UB>())
    return false;

  if (RootDcl->getAddressed())
    return false;

  if (builder.isPreDefArg(RootDcl) || builder.isPreDefRet(RootDcl)) {
    return false;
  }

  if (Dcl->isOutput())
    return false;

  // ToDo: add more special declares to exclude list

  return true;
}

// Associated declarations for splitting.
struct DclMapInfo {
  // The low part of the splitted variable.
  G4_Declare *DclLow;

  // The high part of the splitted variable.
  G4_Declare *DclHigh;

  // Aliases of the low part. Created if needed for different types.
  std::vector<G4_Declare *> AliasLow;

  // Aliases of the high part. Created if needed for different types.
  std::vector<G4_Declare *> AliasHigh;

  DclMapInfo(G4_Declare *Lo, G4_Declare *Hi) : DclLow(Lo), DclHigh(Hi) {}

  // Return an appropriate declaration/alias for low or high part.
  G4_Declare *getDcl(IR_Builder &Builder, G4_Type Ty, bool IsLow) {
    return IsLow ? getDcl(Builder, Ty, DclLow, AliasLow)
                 : getDcl(Builder, Ty, DclHigh, AliasHigh);
  }

private:
  G4_Declare *getDcl(IR_Builder &Builder, G4_Type Ty, G4_Declare *RootDcl,
                     std::vector<G4_Declare *> &Aliases) {
    if (Ty == RootDcl->getElemType())
      return RootDcl;

    for (auto AL : Aliases) {
      if (Ty == AL->getElemType())
        return AL;
    }

    // Create such an alias if it does not exist yet.
    unsigned NElts = RootDcl->getByteSize() / TypeSize(Ty);
    auto Alias = Builder.createTempVar(
        NElts, Ty, Any,
        (std::string(RootDcl->getName()) + "_" + TypeSymbol(Ty)).c_str(),
        false);
    Alias->setAliasDeclare(RootDcl, 0);
    Aliases.push_back(Alias);
    return Alias;
  }
};

} // namespace

//
// We split any 4GRF variables (they typically result from simd16 64-bit vars)
// into two half if
// -- they are not address taken or used in send
// -- none of the operands cross from the 2nd to the 3rd GRF
// This is intended to give RA more freedom as the split variables do
// not have to be allocated contiguously.
// Note that this invalidates existing def-use chains
//
void Optimizer::split4GRFVars() {
  std::unordered_set<G4_Declare *> varToSplit;
  std::vector<G4_Declare *> varToSplitOrdering;
  // map each split candidate to their replacement split variables
  std::unordered_map<const G4_Declare *, DclMapInfo *> DclMap;

  if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) != VISA_3D) {
    return;
  }

  if (builder.getOption(vISA_Debug)) {
    return;
  }

  // Only for simd16 and simd32.
  if (kernel.getSimdSize() == g4::SIMD8) {
    return;
  }

  // first scan the decl list
  for (auto dcl : kernel.Declares) {
    if (dcl->getAliasDeclare() == nullptr) {
      if (isCandidateDecl(dcl, builder)) {
        if (varToSplit.find(dcl) == varToSplit.end()) {
          varToSplitOrdering.push_back(dcl);
        }

        varToSplit.emplace(dcl);
      }
    } else {
      // strictly speaking this condition is not necesary, but having
      // no aliases that could point into a middle of the split candidate
      // makes replacing the split var much easier. By construction the root
      // must appear before its alias decls
      uint32_t offset = 0;
      G4_Declare *rootDcl = dcl->getRootDeclare(offset);
      if (offset != 0 && isCandidateDecl(rootDcl, builder)) {
        varToSplit.erase(rootDcl);
      }
    }
  }

  if (varToSplit.empty()) {
    // early exit if there are no split candidate
    return;
  }

  // first pass is to make sure the validity of all split candidates
  for (auto bb : kernel.fg) {
    for (auto inst : *bb) {
      auto removeCandidate = [&varToSplit](G4_Declare *dcl) {
        if (dcl) {
          dcl = dcl->getRootDeclare();
          varToSplit.erase(dcl);
        }
      };

      if (inst->isSend()) {
        removeCandidate(inst->getDst()->getTopDcl());
        removeCandidate(inst->getSrc(0)->getTopDcl());
        if (inst->isSplitSend()) {
          removeCandidate(inst->getSrc(1)->getTopDcl());
        }
      } else {
        auto cross2GRF = [this](G4_Operand *opnd) {
          uint32_t lb = opnd->getLeftBound();
          uint32_t rb = opnd->getRightBound();
          return (lb < 2u * kernel.numEltPerGRF<Type_UB>()) &&
                 (rb >= 2u * kernel.numEltPerGRF<Type_UB>());
        };
        // check and remove decls with operands that cross 2GRF boundary
        if (inst->getDst()) {
          G4_Declare *dstDcl = inst->getDst()->getTopDcl();
          if (dstDcl && cross2GRF(inst->getDst())) {
            removeCandidate(dstDcl);
          }
        }
        for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
          G4_Operand *src = inst->getSrc(i);
          if (src && src->getTopDcl() && cross2GRF(src)) {
            removeCandidate(src->getTopDcl());
          }
        }
      }
    }
  }

  if (varToSplit.empty()) {
    // early exit if there are no split candidate
    return;
  }

  // create Lo/Hi for each variable being split
  for (auto splitDcl : varToSplitOrdering) {
    // varToSplitOrdering may have extra elements since we never delete any
    // inserted dcl from it.
    if (varToSplit.find(splitDcl) == varToSplit.end())
      continue;
    G4_Type Ty = splitDcl->getElemType();
    unsigned NElts = splitDcl->getTotalElems();
    std::string varName(splitDcl->getName());
    auto DclLow = builder.createTempVar(NElts / 2, Ty, builder.getGRFAlign(),
                                        (varName + "Lo").c_str(), false);
    auto DclHi = builder.createTempVar(NElts / 2, Ty, builder.getGRFAlign(),
                                       (varName + "Hi").c_str(), false);
    DclMap[splitDcl] = new DclMapInfo(DclLow, DclHi);
    // std::cerr << "split " << splitDcl->getName() << " into (" <<
    //    DclLow->getName() << ", " << DclHi->getName() << ")\n";
  }

  // second pass actually does the replacement
  for (auto bb : kernel.fg) {
    for (auto inst : *bb) {
      auto dst = inst->getDst();
      if (dst && dst->getTopDcl()) {
        G4_Declare *dstRootDcl = dst->getTopDcl()->getRootDeclare();
        if (DclMap.count(dstRootDcl)) {
          bool isLow =
              dst->getLeftBound() < 2u * kernel.numEltPerGRF<Type_UB>();
          auto NewDcl =
              DclMap[dstRootDcl]->getDcl(builder, dst->getType(), isLow);
          auto NewDst = builder.createDst(
              NewDcl->getRegVar(), dst->getRegOff() - (isLow ? 0 : 2),
              dst->getSubRegOff(), dst->getHorzStride(), dst->getType(),
              dst->getAccRegSel());
          inst->setDest(NewDst);
        }
      }

      for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
        G4_Operand *src = inst->getSrc(i);
        if (src && src->getTopDcl()) {
          G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
          G4_Declare *srcRootDcl = src->getTopDcl()->getRootDeclare();
          if (DclMap.count(srcRootDcl)) {
            bool isLow =
                src->getLeftBound() < 2u * kernel.numEltPerGRF<Type_UB>();
            auto NewSrcDcl =
                DclMap[srcRootDcl]->getDcl(builder, src->getType(), isLow);
            auto NewSrc = builder.createSrcRegRegion(
                srcRegion->getModifier(), srcRegion->getRegAccess(),
                NewSrcDcl->getRegVar(),
                srcRegion->getRegOff() - (isLow ? 0 : 2),
                srcRegion->getSubRegOff(), srcRegion->getRegion(),
                src->getType(), src->getAccRegSel());
            inst->setSrc(NewSrc, i);
          }
        }
      }
    }
  }

  for (const auto &DI : DclMap) {
    delete DI.second;
  }
}

//
// A platform may not support 64b types (FP64, INT64, or neither).
// While HW conformity should have legalized away use of such types, they may
// get re-introduced again later due to copy moves inserted by spill code
// generation, rematerialization etc. Instead of checking whether 64b type is
// used at each createMov(), we add a catch-all pass here. Since this is called
// post-RA the change we can make are very limited, for now just handle copy
// moves. We make this a separate pass instead of part of changeMoveType() as
// the latter is considered an optimization.
//
void Optimizer::legalizeType() {
  if (builder.noFP64() || builder.noInt64()) {
    for (auto bb : kernel.fg) {
      for (auto inst : *bb) {
        auto uses64bType = [](G4_INST *inst) {
          bool useFP64 = false;
          bool useInt64 = false;
          {
            auto dstTy =
                inst->getDst() ? inst->getDst()->getType() : Type_UNDEF;
            if (dstTy == Type_DF) {
              useFP64 = true;
            } else if (dstTy == Type_Q || dstTy == Type_UQ) {
              useInt64 = true;
            }
          }
          for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
            auto srcTy =
                inst->getSrc(i) ? inst->getSrc(i)->getType() : Type_UNDEF;
            if (srcTy == Type_DF) {
              useFP64 = true;
            } else if (srcTy == Type_Q || srcTy == Type_UQ) {
              useInt64 = true;
            }
          }
          return std::make_tuple(useFP64, useInt64);
        };
        // ToDo: handle more cases (e.g., immSrc, use UD for copy moves)
        if (inst->isRawMov() && inst->getSrc(0)->isSrcRegRegion()) {
          bool hasFP64 = false, hasInt64 = false;
          std::tie(hasFP64, hasInt64) = uses64bType(inst);
          if (hasFP64 && hasInt64) {
            vISA_ASSERT(
                false,
                "can't handle inst with both FP64 and INT64 at this point");
            return;
          }
          if (hasFP64 && builder.noFP64()) {
            vISA_ASSERT(!builder.noInt64(), "can't change DF to UQ");
            inst->getDst()->setType(builder, Type_UQ);
            inst->getSrc(0)->asSrcRegRegion()->setType(builder, Type_UQ);
          }
          if (hasInt64 && builder.noInt64() && !builder.noFP64()) {
            inst->getDst()->setType(builder, Type_DF);
            inst->getSrc(0)->asSrcRegRegion()->setType(builder, Type_DF);
          }
        }
      }
    }
  }
}

//
// Categorize move instructions to help with performance analysis
//
void Optimizer::analyzeMove() {

#define MOVE_TYPE(DO)                                                          \
  DO(Total)                                                                    \
  DO(SatOrMod)                                                                 \
  DO(Imm32)                                                                    \
  DO(Imm64)                                                                    \
  DO(FPConvert)                                                                \
  DO(Trunc)                                                                    \
  DO(Extend)                                                                   \
  DO(Broadcast)                                                                \
  DO(UNPACK)                                                                   \
  DO(PACK)                                                                     \
  DO(Copy)                                                                     \
  DO(Misc)                                                                     \
  DO(LAST)

  enum MovTypes { MOVE_TYPE(MAKE_ENUM) };

  static const char *moveNames[] = {MOVE_TYPE(STRINGIFY)};

  std::array<int, MovTypes::LAST> moveCount = {0};

  for (auto bb : kernel.fg) {
    for (auto inst : *bb) {
      if (!inst->isMov()) {
        continue;
      }
      moveCount[MovTypes::Total]++;

      if (inst->getSaturate()) {
        moveCount[MovTypes::SatOrMod]++;
        continue;
      }
      auto dstTy = inst->getDst()->getType();
      auto srcTy = inst->getSrc(0)->getType();
      if (inst->getSrc(0)->isImm()) {
        moveCount[TypeSize(srcTy) == 8 ? MovTypes::Imm64 : MovTypes::Imm32]++;
      } else if (inst->getSrc(0)->isSrcRegRegion()) {
        auto srcRR = inst->getSrc(0)->asSrcRegRegion();
        if (srcRR->getModifier() != Mod_src_undef) {
          moveCount[SatOrMod]++;
          continue;
        }
        bool signChange = false;
        if (dstTy != srcTy) {
          if (IS_FTYPE(srcTy) || IS_FTYPE(dstTy)) {
            // distinguish inttofp and fpconvert?
            moveCount[MovTypes::FPConvert]++;
          } else if (TypeSize(dstTy) > TypeSize(srcTy)) {
            moveCount[MovTypes::Extend]++;
          } else if (TypeSize(srcTy) > TypeSize(dstTy)) {
            moveCount[MovTypes::Trunc]++;
          } else {
            signChange = true;
          }
        }
        if (dstTy == srcTy || signChange) {
          if (srcRR->isScalar()) {
            moveCount[inst->getExecSize() > g4::SIMD1 ? MovTypes::Broadcast
                                                      : MovTypes::Copy]++;
          } else if (srcRR->getRegion()->isContiguous(inst->getExecSize())) {
            moveCount[inst->getDst()->getHorzStride() == 1
                          ? MovTypes::Copy
                          : MovTypes::UNPACK]++;
          } else {
            bool singleStride =
                srcRR->getRegion()->isSingleStride(inst->getExecSize());
            if (singleStride && inst->getDst()->getHorzStride() == 1) {
              moveCount[MovTypes::PACK]++;
            } else {
              // give up
              moveCount[MovTypes::Misc]++;
            }
          }
        }
      } else {
        moveCount[MovTypes::Misc]++;
      }
    }
  }

  std::cerr << "Move classification:\n";
  for (int i = 0; i < MovTypes::LAST; ++i) {
    if (moveCount[i] > 0) {
      std::cerr << "\t" << moveNames[i] << ":\t" << moveCount[i] << "\n";
    }
  }

#undef MOVE_TYPE
}

void Optimizer::staticProfiling() {
  // NOTE: local data flow analysis can not be called because regVar info
  // missed.
  StaticProfiling s(builder, kernel);
  s.run();

  // Do static cycle profiling only for platforms have 3 or more ALU pipelines.
  // Do static cycle profling only when shader dump is enabled.
  if (builder.hasThreeALUPipes() &&
      builder.getOptions()->getOption(vISA_outputToFile) &&
      builder.getOptions()->getOption(vISA_staticBBProfiling)) {
    StaticCycleProfiling sc(kernel);
    sc.run();
  }
}

static void markBreakpoint(G4_BB *bb, INST_LIST_ITER it, IR_Builder *builder) {
  [[maybe_unused]] G4_INST *inst = *it;
  vISA_ASSERT(inst->isIntrinsic() &&
                  inst->asIntrinsicInst()->getIntrinsicId() ==
                      Intrinsic::Breakpoint,
              "expect breakpoint intrinsic");
  auto nextIt = ++it;
  // if we encounter a breakpoint, mark the instruction after the
  // breakpoint intrinsic with breakpoint instruction option
  if (nextIt != bb->end()) {
    // intrinsic is not at the end of bb
    G4_INST *nextInst = *(nextIt);
    nextInst->setOptionOn(InstOpt_BreakPoint);
  } else {
    // intrinsic is at the end of bb
    // create a dummy mov with breakpoint option set
    auto nullDst = builder->createNullDst(Type_UD);
    auto nullSrc = builder->createNullSrc(Type_UD);

    G4_INST *dummyMov = builder->createMov(g4::SIMD1, nullDst, nullSrc,
                                           InstOpt_BreakPoint, false);
    bb->push_back(dummyMov);
  }
}

//
// remove Intrinsics
//
void Optimizer::removeIntrinsics() {
  for (auto bb : kernel.fg) {
    for (auto I = bb->begin(); I != bb->end(); ++I) {
      G4_INST *inst = *I;
      if (!inst->isIntrinsic())
        continue;
      if (inst->asIntrinsicInst()->getIntrinsicId() == Intrinsic::Breakpoint) {
        markBreakpoint(bb, I, fg.builder);
      }
    }

    std::vector<Intrinsic> intrinIdVec = {
      Intrinsic::MemFence,
      Intrinsic::FlagSpill,
      Intrinsic::Breakpoint
    };
    bb->removeIntrinsics(intrinIdVec);
  }
}

//
// for some platforms int has half throughout compared to float,
// so for copy moves we should change their type
// from D/UD to F or W/UW to HF when possible
//
void Optimizer::changeMoveType() {
  if (!builder.favorFloatMov() && !builder.balanceIntFloatMoves()) {
    return;
  }

  if (builder.avoidSrc1Src2Overlap()) {
    return;
  }

  auto changeType = [this](G4_INST *movInst, G4_Type newTy) {
    movInst->getDst()->setType(builder, newTy);
    auto src0 = movInst->getSrc(0);
    if (src0->isImm()) {
      uint32_t mask = TypeSize(newTy) == 4 ? 0xFFFFFFFF : 0xFFFF;
      movInst->setSrc(
          fg.builder->createImm(src0->asImm()->getImm() & mask, newTy), 0);
      if (newTy == Type_F) {
        uint32_t value = src0->asImm()->getImm() & mask;
        std::stringstream ss;
        ss << "(";
        ss << "0x" << std::setfill('0') << std::hex << std::setw(8) << value;
        ss << ":f)";
        movInst->addComment(ss.str());
      }
    } else {
      movInst->getSrc(0)->asSrcRegRegion()->setType(builder, newTy);
    }
  };

  auto isCandidateMov = [this](G4_INST *inst) {
    if (inst->opcode() != G4_mov || inst->getSaturate() || inst->getCondMod()) {
      return false;
    }
    auto src0 = inst->getSrc(0);
    G4_Type dstTy = inst->getDst()->getType();
    G4_Type src0Ty = src0->getType();
    if (!inst->getDst()->isGreg()) {
      // don't apply it on ARFs (both dst and src)
      return false;
    }
    // Used only for splitting QW->2xUD
    if (TypeSize(dstTy) == 8) {
      if (dstTy != src0Ty) {
        // allow UD->(Q|UQ) as we zext, but not D->Q
        if (!(IS_TYPE_INT(dstTy) && src0Ty == Type_UD)) {
          return false;
        }
      }
      // we can split: scalar, contigous, stride2  w/out SrcMod
      if (src0->isSrcRegRegion() && src0->isGreg()) {
        auto srcReg = src0->asSrcRegRegion();
        bool modifier = srcReg->hasModifier();
        uint16_t stride = 0;
        bool singleStrideMax2 =
            srcReg->getRegion()->isSingleStride(inst->getExecSize(), stride) &&
            (stride <= 2);

        return !modifier && singleStrideMax2;
      } else // or immediates
        return src0->isImm();
    }
    if (dstTy != src0Ty) {
      // allow D <-> UD and W <-> UW moves
      if (!(IS_TYPE_INT(dstTy) && IS_TYPE_INT(src0Ty) &&
            TypeSize(dstTy) == TypeSize(src0Ty))) {
        return false;
      }
    }
    auto isLegalType = [](G4_Type ty) {
      return TypeSize(ty) == 2 || TypeSize(ty) == 4;
    };
    if (!isLegalType(dstTy) || !isLegalType(src0Ty)) {
      return false;
    }

    if (src0->isRelocImm()) {
      return false;
    }

    if (src0->isSrcRegRegion() && src0->isGreg()) {
      auto src0R = src0->asSrcRegRegion();
      bool hasNoModifier = src0R->getModifier() == Mod_src_undef;
      bool hasSimpleRegion =
          src0R->isScalar() ||
          (src0R->getRegion()->isContiguous(inst->getExecSize()) &&
           inst->getDst()->getHorzStride() == 1);
      bool dstSrcAligned =
          src0R->getLinearizedStart() % kernel.numEltPerGRF<Type_UB>() ==
          inst->getDst()->getLinearizedStart() % kernel.numEltPerGRF<Type_UB>();
      return hasNoModifier && hasSimpleRegion && dstSrcAligned;
    } else if (src0->isImm()) {
      // allow sext and zext imm moves
      // float imm can always be converted to int imm
      int64_t immVal = src0->asImm()->getImm();
      bool isIntImmMove = IS_TYPE_INT(dstTy) && IS_TYPE_INT(src0Ty) &&
                          G4_Imm::isInTypeRange(immVal, dstTy);
      return isIntImmMove || IS_FTYPE(dstTy) || IS_HFTYPE(dstTy);
    }
    return false;
  };
  auto splitMov64Imm = [this](INST_LIST_ITER curInst, G4_BB *BB) {
    auto firstMovInst = *curInst;
    auto src0 = firstMovInst->getSrc(0);
    auto dst = firstMovInst->getDst();
    auto srcType = src0->getType();
    auto dstType = dst->getType();

    // Saturate, CondMod, SrcMod, regioning are covered when adding to input
    // list, so no need for check now
    bool isSrcReg = src0->isSrcRegRegion();

    bool isSrcImm = src0->isImm();
    bool is64to64 = isSrcReg && (srcType == dstType) &&
                    (IS_QTYPE(dstType) || dstType == Type_DF);
    bool isU32to64 = isSrcReg && (srcType == Type_UD) &&
                     IS_QTYPE(dstType); // can zero extend it

    if (!(isSrcImm || isU32to64 || is64to64))
      return;

    // common for each variant
    auto newTy = Type_UD;
    unsigned char execSize = firstMovInst->getExecSize();
    unsigned char secondMovExecSize = firstMovInst->getExecSize();

    dst =
        builder.createDst(dst->getBase(), dst->getRegOff(),
                          2 * dst->getSubRegOff(), dst->getHorzStride(), newTy);

    G4_Operand *firstMovSrc0 = src0;
    G4_Operand *secondMovSrc0 = nullptr;

    bool canDoubleExecSize = false;
    if (isSrcImm) {
      uint64_t original = src0->asImm()->getImm();
      uint64_t lopart = original & 0xFFFFFFFF;
      uint64_t hipart = (original >> 32);

      // original mov takes low part
      firstMovSrc0 = fg.builder->createImm(lopart, newTy);

      // second mov, with high part and offset
      secondMovSrc0 = fg.builder->createImm(hipart, newTy);

      /*
          from :
          (W)      mov (8|M0)               r2.0<1>:df    0x0:df

          make:
          (W)      mov (16|M0)              r2.0<1>:ud    0x0:ud
      */
      if (lopart == hipart)
        canDoubleExecSize = true;
    } else if (isU32to64) {
      // original move src0 stays the same (will have different dst)
      // second mov zero extends type
      // TODO(?):  mov r1:uq 0:ud
      secondMovSrc0 = fg.builder->createImm(0, newTy);
    } else if (is64to64) {
      auto src0ASR = src0->asSrcRegRegion();
      auto prevReg = src0ASR->getRegion();

      src0ASR = builder.createSrcRegRegion(
          src0ASR->getModifier(), src0ASR->getRegAccess(), src0ASR->getBase(),
          src0ASR->getRegOff(), src0ASR->getSubRegOff() * 2,
          src0ASR->getRegion(), newTy);

      if (prevReg->vertStride <= 1) {
        // from:
        //       mov (4|M0)               r14.0<1>:q    r24.0<1;1,0>:q
        //       mov (1|M0)               r94.2<1>:q    r14.2<0;1,0>:q
        // to:
        //       mov (8|M0)               r14.0<1>:ud   r24.0<1;1,0>:ud
        //       mov (2|M0)               r94.4<1>:ud   r14.4<1;1,0>:ud
        canDoubleExecSize = true;

        // convert both <0;1,0> and <1;1,0>
        src0ASR->setRegion(builder, fg.builder->getRegionStride1());

        // just create copy of src region to second mov
        secondMovSrc0 = fg.builder->createSubSrcOperand(
            src0ASR, 0, 2 * execSize, 1, prevReg->width);
      } else {
        /* some weird stuff like
               mov (2|M0)               r14.0<1>:q    r24.1<2;1,0>:q

        we should split into 2 (can't double exec).
               mov (2|M0)               r14.0<1>:ud   r24.2<2;1,0>:ud
               mov (2|M0)               r14.1<1>:ud   r24.3<2;1,0>:ud
        */

        // calculate offset on original regioning at lower type
        secondMovSrc0 = fg.builder->createSubSrcOperand(
            src0ASR, 1, execSize, prevReg->vertStride, prevReg->width);

        // change to stride2 now
        auto newReg =
            fg.builder->createRegionDesc(execSize, prevReg->vertStride * 2,
                                         prevReg->width, prevReg->horzStride);

        src0ASR->setRegion(builder, newReg);
        secondMovSrc0->asSrcRegRegion()->setRegion(builder, newReg);
      }
    }
    firstMovInst->setSrc(firstMovSrc0, 0);

    // common offset for all paths
    G4_DstRegRegion *secondMovDst;

    if (canDoubleExecSize) {
      secondMovExecSize *= 2;
      secondMovDst = fg.builder->createSubDstOperand(dst, 0, secondMovExecSize);
    } else {
      secondMovDst = fg.builder->createSubDstOperand(dst, 1, secondMovExecSize);

      // set HzStride for both dst if it matters
      if (execSize > 1) {
        dst->setHorzStride(2);
        secondMovDst->setHorzStride(2);
      }
    }

    G4_Predicate *pred =
        firstMovInst->getPredicate()
            ? builder.duplicateOperand(firstMovInst->getPredicate())
            : nullptr;

    // Create second mov, with different only src/dst, rest the same
    G4_INST *secondMovInst = builder.createInternalInst(
        pred, G4_mov, nullptr, g4::NOSAT, G4_ExecSize(secondMovExecSize),
        secondMovDst, secondMovSrc0, nullptr, firstMovInst->getOption());

    BB->insertBefore(curInst, secondMovInst);

    // we can't alter execSize of first mov, so newMov will take it's place, and
    // remove original
    // TODO: we don't estimate cost of this doubledExec correctly need to fix
    if (canDoubleExecSize) {
      BB->erase(curInst);
    }

    /*
    TODO: currently we do this
    (W)      mov (1|M0)               r66.0<1>:df   0x37F0000000000000:df
    (W)      mov (1|M0)               r66.1<1>:df   0x47F0000000000000:df
    (W)      mov (1|M0)               r66.2<1>:df   0x7FF0000000000000:df
    ->
    (W)      mov (1|M0)               r66.1<1>:ud   0x37F00000:ud
    (W)      mov (1|M0)               r66.0<1>:ud   0x0:ud
    (W)      mov (1|M0)               r66.3<1>:ud   0x47F00000:ud
    (W)      mov (1|M0)               r66.2<1>:ud   0x0:ud
    (W)      mov (1|M0)               r66.5<1>:ud   0x7FF00000:ud
    (W)      mov (1|M0)               r66.4<1>:ud   0x0:ud

    but we could do this ?
    ->
    (W)      mov (1|M0)               r66.1<1>:ud   0x37F00000:ud
    (W)      mov (1|M0)               r66.3<1>:ud   0x47F00000:ud
    (W)      mov (1|M0)               r66.5<1>:ud   0x7FF00000:ud
    (W)      mov (2|M0)               r66.0<2>:ud   0x0:ud
    (W)      mov (1|M0)               r66.4<1>:ud   0x0:ud
    */
  };

  /*
  0 - don't convert.
  1 - per BB balance. <default>
  2 - all suitable 64bit mov (experimental)
  */
  unsigned SplitMov64Mode =
      fg.builder->getOptions()->getuInt32Option(vISA_SplitMov64);

  if (builder.balanceIntFloatMoves()) {
    auto dstOrAnySrcIs2GRF = [this](G4_INST *inst) {
      auto dst = inst->getDst();
      bool dstIs2GRF = dst && !dst->isNullReg() && dst->isCrossGRFDst(builder);
      if (dstIs2GRF)
        return true;

      for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
        auto curSrc = inst->getSrc(i);
        if (inst->getSrc(i) == nullptr)
          continue;
        if (curSrc->isGreg() && curSrc->asSrcRegRegion()->crossGRF(builder))
          return true;
      }
      return false;
    };

    // attempt to balance the number of float v. int instructions in each BB
    // by changing the types of int or float copy moves
    for (auto bb : fg) {
      // candidate int and float moves
      std::vector<G4_INST *> intMovs, floatMovs;
      std::vector<INST_LIST_ITER> QWInstructions;
      // int/math/send share one decoder, float and 64b share the other decoder
      int numIntCost = 0, numFloatCost = 0;
      for (auto I = bb->begin(); I != bb->end(); /*empty*/) {
        auto CurI = I++;
        G4_INST *inst = *CurI;
        if (inst->getDst() && !inst->isDpas()) {
          auto execSize = inst->getExecSize();
          G4_Type dstTy = inst->getDst()->getType();
          uint32_t dstTySize = TypeSize(dstTy);

          uint32_t affectedGRFsCost = dstOrAnySrcIs2GRF(inst) ? 2 : 1;

          // Assumption:
          // FPU0 : FLT16/FLT32/FLT64/INT64
          // FPU1 : INT16 / INT32 / EM
          if (inst->isMath()) {
            // native simd1 for :DF, simd2 for :F
            numIntCost += (dstTySize == 8) ? execSize : execSize / 2;
          } else if (inst->isSend()) {
            numIntCost++;
          } else if (dstTySize == 8) {
            numFloatCost += affectedGRFsCost;
            if (isCandidateMov(inst)) {
              QWInstructions.push_back(CurI);
            }
          } else {
            if (IS_TYPE_INT(dstTy)) {
              numIntCost += affectedGRFsCost;
              if (isCandidateMov(inst)) {
                intMovs.push_back(inst);
              }
            } else if (IS_TYPE_FLOAT_ALL(dstTy)) {
              numFloatCost += affectedGRFsCost;
              if (isCandidateMov(inst)) {
                floatMovs.push_back(inst);
              }
            }
          }
        }
      }
      // std::cout << "num int cost/mov: " << numIntCost << "/" <<
      // intMovs.size() << " "
      //           << "num float cost/mov: " << numFloatCost << "/" <<
      //           floatMovs.size() << " "
      //           << "QW movs: " << QWInstructions.size() << "\n";
      int diff = std::abs(numIntCost - numFloatCost) / 2;

      auto changeMovsFromVector = [&](std::vector<G4_INST *> &table,
                                      G4_Type newType32, G4_Type newType16) {
        for (int i = 0, numInt = table.size(); diff > 0 && i < numInt; ++i) {
          auto inst = table[i];
          auto typeSize = inst->getDst()->getTypeSize();
          G4_Type floatTy = typeSize == 4 ? newType32 : newType16;

          changeType(inst, floatTy);

          auto estimatedClockCount = dstOrAnySrcIs2GRF(inst) ? 2 : 1;
          diff -= estimatedClockCount;
        }
      };

      bool forceSplitAllMov64 = (SplitMov64Mode == 2);

      if (numIntCost > numFloatCost && !forceSplitAllMov64) {
        // change int move to float move
        changeMovsFromVector(intMovs, Type_F, Type_HF);
      } else {
        // change float move to int move
        changeMovsFromVector(floatMovs, Type_UD, Type_UW);

        // if there's still unbalance
        // split `mov <imm64>` (or `mov 64to64` or mov `u32to64`) into 2x `mov
        // <imm32>`
        // TODO: or maybe split "and", "or" as well

        // Above changeMovsFromVector() had always same added and decreased
        // values so it operated on halfDiff but now we might have different
        // values so let's operate on full diff, not half
        diff = diff * 2;

        int rep = 0;
        if (!SplitMov64Mode)
          diff = 0;

        for (int i = 0, numInt = QWInstructions.size();
             ((diff > 0) || forceSplitAllMov64) && i < numInt; ++i) {
          auto inst = *QWInstructions[i];
          auto execSize = inst->getExecSize();
          auto estimatedSrcCost =
              dstOrAnySrcIs2GRF(inst) ? 2 : 1; // cost of mov before change

          auto dstTypeSize = TypeSize(Type_UD);
          auto estimatedDstCost = (execSize * dstTypeSize * /*HzStride*/ 2) > 32
                                      ? 2
                                      : 1; // cost of new mov

          // it might be that we remove 1 cycle mov (1) :df, and add 2x 1cycle
          // mov(1) :ud => 3 cycles diff.
          auto new_diff = diff - (2 * estimatedDstCost + estimatedSrcCost);

          if (abs(new_diff) >= abs(diff) && !forceSplitAllMov64) {
            break;
          }

          splitMov64Imm(QWInstructions[i], bb);

          diff = new_diff;
          rep++;
        }
        // std::cout << "diff before " << diff_prev << " after " << diff <<"
        // reps done " << rep << "\n";
      }
    }
    return;
  }

  for (auto bb : fg) {
    for (auto inst : *bb) {
      if (inst->opcode() != G4_mov) {
        continue;
      }
      // copy move means dst and src has identical bits (implies same type
      // width), and there are no sat/conditional modifier as well as src
      // modifier ToDo: we should probably change isRawMov() to include mov UD D
      auto src0 = inst->getSrc(0);

      // FIXME: This is a quick WA to bypass RelocImm, so that it won't create a
      // new src0 and overwrite the original RelocImm While this optimization
      // should still be able to apply to RelocImm. Once we turn on this
      // optimization for RelocImm, we should update assert in
      // VISAKernelImpl::GetGenRelocEntryBuffer to allow float type
      if (src0->isRelocImm())
        continue;

      G4_Type dstTy = inst->getDst()->getType();
      G4_Type src0Ty = src0->getType();
      bool hasNoModifier =
          !inst->getSaturate() && !inst->getCondMod() &&
          (src0->isImm() ||
           (src0->isSrcRegRegion() &&
            src0->asSrcRegRegion()->getModifier() == Mod_src_undef));

      // it may be unsafe to change the move type for acc as it has higher
      // precision
      if (inst->getDst()->isGreg() && hasNoModifier) {
        if (src0->isGreg()) {
          bool isIntCopyMove = IS_TYPE_INT(dstTy) && IS_TYPE_INT(src0Ty) &&
                               TypeSize(dstTy) == TypeSize(src0Ty);
          if (isIntCopyMove) {
            if (dstTy == Type_D || dstTy == Type_UD) {
              changeType(inst, Type_F);
            } else if (dstTy == Type_W || dstTy == Type_UW) {
              changeType(inst, Type_HF);
            }
          }
        } else if (src0->isImm()) {
          // allow sext and zext imm moves
          int64_t immVal = src0->asImm()->getImm();
          bool isIntImmMove = IS_TYPE_INT(dstTy) && IS_TYPE_INT(src0Ty) &&
                              G4_Imm::isInTypeRange(immVal, dstTy);
          if (isIntImmMove) {
            if (dstTy == Type_D || dstTy == Type_UD) {
              changeType(inst, Type_F);
            } else if (dstTy == Type_W || dstTy == Type_UW) {
              changeType(inst, Type_HF);
            }
          }
        }
      }
    }
  }
}

static bool isDeadInst(FlowGraph &fg, G4_INST *Inst) {
  if ((Inst->isMov() && !Inst->isRelocationMov() &&
       !Inst->getDst()->isNullReg()) ||
      Inst->isLogic() || Inst->isCompare() || Inst->isArithmetic() ||
      Inst->isVector()) {

    // Check side-effects.
    // - no global
    // - no indirect
    // - not physically assigned (including ARF)
    auto checkOpnd = [&](G4_Operand *Opnd) {
      if (Opnd == nullptr || Opnd->isNullReg())
        return true;
      if (fg.globalOpndHT.isOpndGlobal(Opnd))
        return false;
      if (Opnd->isDstRegRegion() && Opnd->asDstRegRegion()->isIndirect())
        return false;
      if (G4_VarBase *Base = Opnd->getBase()) {
        if (!Base->isRegVar())
          return false;
        if (Base->asRegVar()->isPhyRegAssigned())
          return false;
      }
      if (G4_Declare *Dcl = Opnd->getTopDcl()) {
        if (Dcl->isPreDefinedVar()) {
          // This can be improved by checking each preDefinedVar
          return false;
        }
        if (Dcl->isOutput() || Dcl->isPayloadLiveOut())
          return false;
      }
      return true;
    };

    // Should have no use.
    if (Inst->use_size() > 0)
      return false;

    // Skip instructions with special attributes.
    if (Inst->isYieldInst() || Inst->isBreakPointInst())
      return false;

    // Check defs. Assuming acc operands are all locally defined
    // and def-use are correctly maintained.
    if (!checkOpnd(Inst->getDst()) || !checkOpnd(Inst->getCondMod()))
      return false;

    // At this point, this instruction is dead.
    return true;
  }

  // By default it is not dead.
  return false;
}

// DCE() is disabled if the kernel has non-LSC messages because of below issue:
// Some cases have inaccurate kills, thus it is unsafe to turn it on by default.
// For example,
//    1. mov (1) r10.2:ud  r 20.0:ud
//    2. send (1) r10:ud  ...  // a32 dword read
//    3.   ... r10.2 ...
// In this case, send's footprint is the entire r10 (0-7 dw), thus it kill 1).
// In fact, send only modifies r10.0:ud (a single dw), thus it actually does not
// kill 1).  If dce is on, it willl use the false kill info to remove 1), as
// result, the code would be wrong.
//
//
void Optimizer::dce() {
  // Do not do DCE if there is any legacy message.
  for (auto bb : fg) {
    for (auto I = bb->rbegin(), E = bb->rend(); I != E; ++I) {
      G4_INST *Inst = *I;
      if (Inst->isSend() && Inst->getMsgDesc()->isHDC()) {
        return;
      }
    }
  }

  // make sure dataflow is up to date
  kernel.fg.resetLocalDataFlowData();
  kernel.fg.localDataFlowAnalysis();

  for (auto bb : fg) {
    for (auto I = bb->rbegin(), E = bb->rend(); I != E; ++I) {
      G4_INST *Inst = *I;
      if (isDeadInst(fg, Inst)) {
        Inst->removeAllDefs();
        Inst->markDead();
      }
    }
    bb->erase(std::remove_if(bb->begin(), bb->end(),
                             [](G4_INST *Inst) { return Inst->isDead(); }),
              bb->end());
  }
}

// Barrier is translated into signal and wait instructions. Both are scheduling
// barriers resulting in no any other instruction could be scheduled in-between.
// However, signal will take a while to go through among threads, so we could
// treat wait as a scheduling barrier for mayLoad/mayStore instructions only to
// improve performance. This pass tries to sink barrier wait until non-scratch
// send is found or when there's a possible flag overlap for nbarrier cases.
void Optimizer::sinkBarrierWait() {
  // Skip the optimization when barrier WA is required.
  if (builder.needBarrierWA())
    return;

  // TODO: Check whether there's really a flag overlap between the two
  // instructions. Given named barrier with flag src0 probably is rarely used,
  // currently simply treat the case, wait's src0 is flag and the current inst
  // writes flag, as an overlap.
  auto hasFlagOverlap = [](const G4_INST *i1, const G4_INST *i2) -> bool {
    vASSERT(i1 && i1->opcode() == G4_wait);
    return i1->getSrc(0)->isFlag() && i2->writesFlag();
  };

  for (auto bb : fg) {
    INST_LIST waits;
    for (auto it = bb->begin(), ie = bb->end(); it != ie;) {
      G4_INST *inst = *it;
      // Move any barrier wait to the temporary list.
      if (inst->opcode() == G4_wait) {
        auto next = std::next(it);
        waits.splice(waits.end(), bb->getInstList(), it);
        it = next;
        continue;
      }

      // Move all barrier waits from the temporary list back to inst list right
      // before an interesting position like a non-scratch send or
      // a control-flow instruction.
      // TODO: Check if we can relax or need more restrictions. For example,
      // private memory access probably could also be skipped.
      if ((inst->isSend() && !inst->getMsgDesc()->isScratch()) ||
          inst->isCFInst())
        bb->splice(it, waits);

      // When there's any wait that has a flag overlap with the current inst,
      // move the range [waits.begin(), last overlapping wait iterator] back to
      // the inst list so that the waits are not reordered.
      auto rwit = std::find_if(waits.rbegin(), waits.rend(),
          [=](const G4_INST *i) { return hasFlagOverlap(i, inst); });
      if (rwit != waits.rend()) {
        G4_INST *prev = nullptr, *last = *rwit;
        auto wit = waits.begin();
        while (prev != last) {
          prev = *wit;
          auto next = std::next(wit);
          bb->splice(it, waits, wit);
          wit = next;
        };
      }

      ++it;
    }
    // Every BB should end with a EOT or a CF inst like goto/jmpi/ret.
    vASSERT(waits.empty());
  }
}
