/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Assertions.h"
#include "FlowGraph.h"
#include "G4_Opcode.h"
#include "G4_Verifier.hpp"
#include "Optimizer.h"
#include "PointsToAnalysis.h"
#include "Timer.h"
#include "visa_igc_common_header.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

using namespace vISA;

// A place for all software workarounds for HW issues. Future work may be to
// move large SWWAs into their own pass instead of inside Optimizer.

// Various helper functions for creating dummy instructions that may assist in
// SW workarounds.
void Optimizer::insertDummyCompactInst() {
  // Only for SKL+ and compaction is enabled.
  if (builder.getPlatform() < GENX_SKL || !builder.getOption(vISA_Compaction))
    return;

  // Insert mov (1) r0 r0 at the beginning of this kernel.
  G4_Declare *dcl = builder.getBuiltinR0();
  auto src = builder.createSrc(dcl->getRegVar(), 0, 0,
                               builder.getRegionScalar(), Type_F);
  auto dst = builder.createDst(dcl->getRegVar(), 0, 0, 1, Type_F);
  G4_INST *movInst =
      builder.createMov(g4::SIMD1, dst, src, InstOpt_WriteEnable, false);

  auto bb = fg.getEntryBB();
  for (auto it = bb->begin(), ie = bb->end(); it != ie; ++it) {
    if ((*it)->opcode() != G4_label) {
      bb->insertBefore(it, movInst);
      return;
    }
  }

  // The entry block is empty or only contains a label.
  bb->push_back(movInst);
}

void Optimizer::swapSrc1Src2OfMadForCompaction() {
  if (!builder.src1Src2SwapForCompaction())
    return;

  BB_LIST_ITER ib, bend(fg.end());
  for (ib = fg.begin(); ib != bend; ++ib) {
    G4_BB *bb = (*ib);
    INST_LIST_ITER ii = bb->begin();

    while (ii != bb->end()) {
      G4_INST *inst = *ii;
      if (inst->opcode() == G4_mad) {
        G4_Operand *src1 = inst->getSrc(1);
        G4_Operand *src2 = inst->getSrc(2);
        if (src1 && src2 && src1->getType() == src2->getType() &&
            src1->isSrcRegRegion() &&
            src2->isSrcRegRegion() &&
            src1->getBase()->isRegVar() && src2->getBase()->isRegVar() &&
            src1->getTopDcl()->getRegFile() == G4_GRF &&
            src2->getTopDcl()->getRegFile() == G4_GRF) {
          if (src1->asSrcRegRegion()->getRegion()->isScalar() &&
              src2->asSrcRegRegion()->getRegion()->isFlatRegion()) {
            inst->setSrc(src2, 1);
            inst->setSrc(src1, 2);
          }
        }
      }
      ii++;
    }
  }
}

// add (1|M0)   null<1>:uw   null<0;1,0>:uw       0x0:uw
void Optimizer::insertDummyAdd(G4_BB *bb, INST_LIST_ITER inst_it, int imm) {
  // Dst
  auto nullDst = builder.createNullDst(Type_UW);
  auto nullSrc0 = builder.createNullSrc(Type_UW);
  auto immSrc1 = builder.createImm(imm, Type_UW);

  auto addInst = builder.createBinOp(G4_add, g4::SIMD1, nullDst, nullSrc0,
                                     immSrc1, InstOpt_WriteEnable, false);

  bb->insertBefore(inst_it, addInst);
}

// Float and DP share same GRF cache.
// Integer and Math shader same GRF cache.
void Optimizer::insertDummyMad(G4_BB *bb, INST_LIST_ITER inst_it) {
  // Dst
  auto nullDst1 = builder.createNullDst(Type_W);
  auto nullDst2 = builder.createNullDst(Type_F);

  const RegionDesc *region = builder.createRegionDesc(8, 8, 1);

  // Src0
  auto src0Dcl_0 = builder.createHardwiredDeclare(1, Type_W, 1, 0);
  auto src0Dcl_1 = builder.createHardwiredDeclare(1, Type_F, 1, 0);
  G4_SrcRegRegion *src0Opnd_0 =
      kernel.fg.builder->createSrcRegRegion(src0Dcl_0, region);
  G4_SrcRegRegion *src0Opnd_1 =
      kernel.fg.builder->createSrcRegRegion(src0Dcl_1, region);

  G4_SrcRegRegion *src1Opnd_0 =
      kernel.fg.builder->createSrcRegRegion(src0Dcl_0, region);
  G4_SrcRegRegion *src1Opnd_1 =
      kernel.fg.builder->createSrcRegRegion(src0Dcl_1, region);

  G4_SrcRegRegion *src2Opnd_0 =
      kernel.fg.builder->createSrcRegRegion(src0Dcl_0, region);
  G4_SrcRegRegion *src2Opnd_1 =
      kernel.fg.builder->createSrcRegRegion(src0Dcl_1, region);

  auto madInst1 = builder.createInternalInst(
      nullptr, G4_mad, nullptr, g4::NOSAT, g4::SIMD8, nullDst1, src0Opnd_0,
      src1Opnd_0, src2Opnd_0, InstOpt_NoOpt);

  auto madInst2 = builder.createInternalInst(
      nullptr, G4_mad, nullptr, g4::NOSAT, g4::SIMD8, nullDst2, src0Opnd_1,
      src1Opnd_1, src2Opnd_1, InstOpt_NoOpt);

  bb->insertBefore(inst_it, madInst1);
  bb->insertBefore(inst_it, madInst2);

  G4_SrcRegRegion *src =
      kernel.fg.builder->createSrcRegRegion(src0Dcl_1, region);
  G4_DstRegRegion *dst = kernel.fg.builder->createDstRegRegion(src0Dcl_1, 1);
  G4_INST *movInst =
      builder.createMov(g4::SIMD8, dst, src, InstOpt_NoOpt, false);

  bb->insertBefore(inst_it, movInst);
}

void Optimizer::insertDummyCsel(G4_BB *bb, INST_LIST_ITER inst_it, bool newBB) {
  const RegionDesc *region = builder.createRegionDesc(4, 4, 1);

  G4_Declare *dummyFlagDcl = builder.createTempFlag(1, "dmflag");
  dummyFlagDcl->getRegVar()->setPhyReg(builder.phyregpool.getFlagAreg(0), 0);
  auto dummyCondMod0 =
      builder.createCondMod(Mod_e, dummyFlagDcl->getRegVar(), 0);
  auto src0Dcl_0 = builder.createHardwiredDeclare(4, Type_W, 1, 0);
  G4_SrcRegRegion *src0Opnd_0 =
      kernel.fg.builder->createSrcRegRegion(src0Dcl_0, region);
  G4_SrcRegRegion *src1Opnd_0 =
      kernel.fg.builder->createSrcRegRegion(src0Dcl_0, region);
  G4_SrcRegRegion *src2Opnd_0 =
      kernel.fg.builder->createSrcRegRegion(src0Dcl_0, region);
  G4_DstRegRegion *dst0 = kernel.fg.builder->createDstRegRegion(src0Dcl_0, 1);
  auto cselInst0 = builder.createInternalInst(
      nullptr, G4_csel, dummyCondMod0, g4::NOSAT, g4::SIMD4, dst0, src0Opnd_0,
      src1Opnd_0, src2Opnd_0, InstOpt_WriteEnable);

  if (newBB) {
    bb->push_back(cselInst0);
  } else {
    bb->insertBefore(inst_it, cselInst0);
  }

  if (!builder.hasSingleALUPipe()) {
    auto src0Dcl_1 = builder.createHardwiredDeclare(4, Type_F, 1, 4);
    G4_SrcRegRegion *src0Opnd_1 =
        kernel.fg.builder->createSrcRegRegion(src0Dcl_1, region);
    G4_SrcRegRegion *src1Opnd_1 =
        kernel.fg.builder->createSrcRegRegion(src0Dcl_1, region);
    G4_SrcRegRegion *src2Opnd_1 =
        kernel.fg.builder->createSrcRegRegion(src0Dcl_1, region);
    G4_DstRegRegion *dst1 = kernel.fg.builder->createDstRegRegion(src0Dcl_1, 1);
    auto dummyCondMod1 =
        builder.createCondMod(Mod_e, dummyFlagDcl->getRegVar(), 0);
    auto cselInst1 = builder.createInternalInst(
        nullptr, G4_csel, dummyCondMod1, g4::NOSAT, g4::SIMD4, dst1, src0Opnd_1,
        src1Opnd_1, src2Opnd_1, InstOpt_WriteEnable);
    if (newBB) {
      bb->push_back(cselInst1);
    } else {
      bb->insertBefore(inst_it, cselInst1);
    }
  }
}

void Optimizer::insertDummyMov(G4_BB *bb, INST_LIST_ITER inst_it,
                               G4_Operand *opnd) {
  G4_SrcRegRegion *src =
      builder.createSrc(opnd->getBase(), opnd->asSrcRegRegion()->getRegOff(), 0,
                        builder.createRegionDesc(8, 8, 1), Type_UD);
  G4_DstRegRegion *dst = builder.createDst(
      opnd->getBase(), opnd->asSrcRegRegion()->getRegOff(), 0, 1, Type_UD);
  G4_INST *movInst =
      builder.createMov(g4::SIMD8, dst, src, InstOpt_NoOpt, false);
  bb->insertBefore(inst_it, movInst);

  return;
}

void Optimizer::insertDummyMovForHWRSWADPAS(G4_BB *bb) {
  INST_LIST_ITER curr_iter = bb->begin();
  bool PreDPAS = false;
  while (curr_iter != bb->end()) {
    G4_INST *inst = (*curr_iter);

    if (inst->isDpas() &&
        !PreDPAS) // Within a BB, only first one need invalid DPAS suppresion
    {
      insertDummyMov(bb, curr_iter, inst->getSrc(1));
      PreDPAS = true;
    }

    if (inst->getPredicate() && inst->getDst() &&
        !inst->getDst()->isNullReg()) {
      if (inst->isSend()) {
        PreDPAS = false;
      }
    }

    ++curr_iter;
  }
}

void Optimizer::insertDummyMovForHWRSWAonaAllpipelines() {
  bool hasNonUniformBranch = false;
  bool hasPredicatedSendOrIndirect = false;
  BB_LIST dpasBBs;

  for (BB_LIST_ITER bb_it = kernel.fg.begin(); bb_it != kernel.fg.end();
       bb_it++) {
    G4_BB *bb = (*bb_it);

    if (bb->empty()) {
      continue;
    }

    INST_LIST_ITER curr_iter = bb->begin();
    INST_LIST_ITER pre_iter = curr_iter;
    bool insertDPASBB = false;
    while (curr_iter != bb->end()) {
      G4_INST *inst = (*curr_iter);

      if (inst->isDpas() && !insertDPASBB) {
        dpasBBs.push_back(bb);
        insertDPASBB = true;
      }

      if (inst->getPredicate() && inst->getDst() &&
          !inst->getDst()->isNullReg()) {
        if (inst->isSend()) {
          insertDummyCsel(bb, curr_iter, false);
          hasPredicatedSendOrIndirect = true;
        }
      }

      if (builder.hasEOTReadSuppressionIssue() && inst->isEOT()) {
        if (pre_iter != curr_iter) {
          G4_INST *pre_inst = (*pre_iter);
          if (pre_inst->isAtomicInst()) {
            insertDummyCsel(bb, pre_iter, false);
          } else {
            insertDummyCsel(bb, curr_iter, false);
          }
        }
      }

      pre_iter = curr_iter;
      ++curr_iter;
    }

    bool newBB = false;
    G4_INST *inst = (bb->getInstList().back());
    if (inst->isRSWADivergentInst() && !inst->asCFInst()->isUniform()) {
      bool previousElse = false;

      G4_BB *preBB = bb->getPhysicalPred();
      if (preBB && preBB->getInstList().size()) {
        G4_INST *preBBLastInst = (preBB->getInstList().back());
        previousElse = (preBBLastInst->opcode() == G4_else);
      }

      INST_LIST_ITER iter = bb->end();
      iter--;
      if (iter != bb->begin() && !previousElse) {
        INST_LIST_ITER preIter = iter;
        preIter--;
        G4_INST *preInst = (*preIter);
        if (preInst->isLabel()) {
          bool hasJmpIPred = false;

          for (G4_BB *predBB : bb->Preds) {
            G4_INST *predBBLastInst = NULL;
            if (!predBB->empty()) {
              predBBLastInst = predBB->getInstList().back();
            }
            if (predBBLastInst && predBBLastInst->opcode() == G4_jmpi) {
              hasJmpIPred = true;
            }
          }
          G4_BB *wa_bb = hasJmpIPred ? kernel.fg.createNewBBWithLabel("RSWA")
                                     : kernel.fg.createNewBB();
          kernel.fg.insert(bb_it, wa_bb);
          G4_Label *newLabel = hasJmpIPred ? wa_bb->getLabel() : NULL;

          // replace bb with wa_bb in the pred BB of bb.
          for (G4_BB *predBB : bb->Preds) {
            G4_INST *predBBLastInst = NULL;
            if (!predBB->empty()) {
              predBBLastInst = predBB->getInstList().back();
            }
            if (predBBLastInst && predBBLastInst->opcode() == G4_jmpi) {
              vASSERT(newLabel);
              predBBLastInst->setSrc(newLabel, 0);
            }

            // C++17: std::replace(predBB->Succs.begin(), predBB->Succs.end(),
            // bb, wa_bb);
            for (G4_BB *&succ : predBB->Succs) {
              if (succ == bb) {
                succ = wa_bb;
              }
            }
            wa_bb->Preds.push_back(predBB);
          }
          wa_bb->Succs.push_back(bb);
          bb->Preds.clear();
          bb->Preds.push_back(wa_bb);
          newBB = true;
          bb = wa_bb;
        }
      }

      insertDummyCsel(bb, iter, newBB);
      hasNonUniformBranch = true;
    }
  }

  if (dpasBBs.size() &&
      builder.getOptions()->getOption(vISA_InsertDummyMovForDPASRSWA) &&
      (hasPredicatedSendOrIndirect || hasNonUniformBranch)) {
    for (G4_BB *bb : kernel.fg) {
      insertDummyMovForHWRSWADPAS(bb);
    }
  }
}

void Optimizer::insertDummyMovForHWRSWAonDPAS() {
  bool hasNonUniformBranch = false;
  bool hasPredicatedSendOrIndirect = false;
  BB_LIST dpasBBs;

  for (BB_LIST_ITER bb_it = kernel.fg.begin(); bb_it != kernel.fg.end();
       bb_it++) {
    G4_BB *bb = (*bb_it);

    if (bb->empty()) {
      continue;
    }

    INST_LIST_ITER curr_iter = bb->begin();
    bool insertDPASBB = false;
    while (curr_iter != bb->end()) {
      G4_INST *inst = (*curr_iter);

      if (inst->isDpas() && !insertDPASBB) {
        dpasBBs.push_back(bb);
        insertDPASBB = true;
      }

      if (inst->getPredicate() && inst->getDst() &&
          !inst->getDst()->isNullReg()) {
        if (inst->isSend()) {
          hasPredicatedSendOrIndirect = true;
        }
      }

      ++curr_iter;
    }

    G4_INST *inst = (bb->getInstList().back());
    if (inst->isRSWADivergentInst() && !inst->asCFInst()->isUniform()) {
      hasNonUniformBranch = true;
    }
  }

  if (dpasBBs.size() &&
      builder.getOptions()->getOption(vISA_InsertDummyMovForDPASRSWA) &&
      (hasPredicatedSendOrIndirect || hasNonUniformBranch)) {
    for (G4_BB *bb : dpasBBs) {
      insertDummyMovForHWRSWADPAS(bb);
    }
  }
}

void Optimizer::insertDummyMovForHWRSWA() {
  if (!((VISA_WA_CHECK(builder.getPWaTable(), Wa_16012061344) ||
         VISA_WA_CHECK(builder.getPWaTable(), Wa_22012856258) ||
         VISA_WA_CHECK(builder.getPWaTable(), Wa_14017322320) ||
         VISA_WA_CHECK(builder.getPWaTable(), Wa_16012292205)))) {
    return;
  }

  if (builder.hasRSForSpecificPlatform()) {
    insertDummyMovForHWRSWAonaAllpipelines();
  } else {
    insertDummyMovForHWRSWAonDPAS();
  }
}

// 1. set DMask so that upper 16bits are ones.
//    This may be done in applyFusedCallWA(). Doing so here has minimum impact
//    to visa.
// 2. Perform IP WA if needed.
void Optimizer::finishFusedCallWA_preSWSB() {
  if (builder.getIsKernel()) {
    // If it is from scalar IGC, need to extend its dmask. For example, simd8 to
    // simd16 or simd16 to simd32 by adding or instructions on the entry.  Note
    // that the first BB is not necessarily the kernel's entry when kernel needs
    // to load its payload!
    //    (W) or (1|M0)  dmask(sr0.2)  dmasksr0.2  0xFFFF0000
    if (true /*kernel.getInt32KernelAttr(Attributes::ATTR_Target) != VISA_CM */)
        {
      // Use M16 always.
      vASSERT(kernel.getSimdSize() <= 16);
      uint32_t orImm = kernel.getSimdSize() == 16 ? 0xFFFF0000 : 0xFFFFFF00;

      G4_VarBase *V_sr0 = builder.phyregpool.getSr0Reg();
      G4_SrcRegRegion *I0_Src0 =
          builder.createSrc(V_sr0, 0, 2, builder.getRegionScalar(), Type_UD);
      G4_Imm *newDMask = builder.createImm(orImm, Type_UD);
      G4_DstRegRegion *I0_Dst = builder.createDst(V_sr0, 0, 2, 1, Type_UD);
      G4_INST *I0 = builder.createInternalInst(
          nullptr, G4_or, nullptr, g4::NOSAT, g4::SIMD1, I0_Dst, I0_Src0,
          newDMask, InstOpt_WriteEnable);

      G4_BB *entryBB = fg.getEntryBB();
      // Make sure to skip prolog BBs to insert into the 1st BB of a kernel.
      G4_BB *perThreadBB = kernel.getPerThreadPayloadBB();
      G4_BB *crossThreadBB = kernel.getCrossThreadPayloadBB();
      if (perThreadBB != nullptr || crossThreadBB != nullptr) {
        while (entryBB != nullptr) {
          if (entryBB == perThreadBB || entryBB == crossThreadBB) {
            // perthread/crossThread BB has a single succ.
            vASSERT(entryBB->Succs.size() == 1);
            entryBB = entryBB->Succs.front();
            continue;
          }
          break;
        }
      }
      entryBB->insertBefore(entryBB->getFirstInsertPos(), I0);
    }
  }

  if (kernel.m_indirectCallWAInfo.empty() && kernel.m_maskOffWAInsts.empty())
    return;

#if defined(_DEBUG)
  // Expect all BBs and insts related to call wa are present and the insts are
  // still in their BBs (they could be reordered, but are required to be in the
  // original BB).
  //
  // Don't expect any violation, but do the sanity check here to make sure.
  for (auto &II : kernel.m_indirectCallWAInfo) {
    G4_BB *BB = II.first;
    IndirectCallWAInfo &callWAInfo = II.second;
    G4_BB *BigBB = callWAInfo.Big_BB;
    G4_BB *SmallBB = callWAInfo.Small_BB;
    if (std::find(kernel.fg.begin(), kernel.fg.end(), BB) == kernel.fg.end() ||
        std::find(kernel.fg.begin(), kernel.fg.end(), BigBB) ==
            kernel.fg.end() ||
        std::find(kernel.fg.begin(), kernel.fg.end(), SmallBB) ==
            kernel.fg.end()) {
      vISA_ASSERT(false, "ICE: BB not found in indirect call WA info!");
      break;
    }

    G4_INST *ip_wa = callWAInfo.IP_WA_placeholder;
    G4_INST *bigStart = callWAInfo.Big_start;
    G4_INST *bigPatch = callWAInfo.Big_patch;
    G4_INST *smallStart = callWAInfo.Small_start;
    G4_INST *smallPatch = callWAInfo.Small_patch;
    G4_INST *bigCall = callWAInfo.Big_call;
    G4_INST *smallCall = callWAInfo.Small_call;
    if ((ip_wa && std::find(BB->begin(), BB->end(), ip_wa) == BB->end()) ||
        (bigStart &&
         std::find(BB->begin(), BB->end(), bigStart) == BB->end()) ||
        (bigPatch &&
         std::find(BB->begin(), BB->end(), bigPatch) == BB->end()) ||
        (smallStart &&
         std::find(BB->begin(), BB->end(), smallStart) == BB->end()) ||
        (smallPatch &&
         std::find(BB->begin(), BB->end(), smallPatch) == BB->end()) ||
        (bigCall &&
         std::find(BigBB->begin(), BigBB->end(), bigCall) == BigBB->end()) ||
        (smallCall && std::find(SmallBB->begin(), SmallBB->end(), smallCall) ==
                          SmallBB->end())) {
      vISA_ASSERT(false, "ICE: inst not found in its original BB!");
      break;
    }
  }

  for (const auto& II : kernel.m_maskOffWAInsts) {
    G4_INST *tInst = II.first;
    G4_BB *tBB = II.second;

    // make sure BB and inst are still valid
    if (std::find(kernel.fg.begin(), kernel.fg.end(), tBB) == kernel.fg.end()) {
      vISA_ASSERT(false, "ICE: BB not in m_maskOffWAInsts!");
      continue;
    }
    if (std::find(tBB->begin(), tBB->end(), tInst) == tBB->end()) {
      vISA_ASSERT(false, "ICE: inst not in m_maskOffWAInsts!");
      continue;
    }
  }
#endif

  if (builder.needIPWA()) {
    for (auto &II : kernel.m_indirectCallWAInfo) {
      G4_BB *BB = II.first;
      IndirectCallWAInfo &callWAInfo = II.second;

      G4_INST *ip_wa = callWAInfo.IP_WA_placeholder;
      if (ip_wa == nullptr) {
        // calla, ip wa not needed.
        continue;
      }

      G4_INST *ip_inst = nullptr;
      if (ip_wa) {
        // clang-format off
        // Simplified example to show what it does:
        //  Given
        //       pseudo_fcall (16)    r4.0:ud
        //
        //  After applyFusedCallWA and RA:
        //   (W) mov (1)   r2.0<1>:ud  sr0.0<0;1,0>:ud
        //   (W) and (16) (eq)f1.0 null<1>:uw  r2.0<0;1,0>:uw  0x80:uw
        //   (W&!f1.0) mov (1)     cr0.2<1>:ud  r4.0<0;1,0>:ud
        //   (W) mov (1)   r3.2<1>:ud  cr0.2<0;1,0>:ud
        //   (W) mov (1)   r3.0<1>:d  0x89abcdef:d          :ip_wa (placeholder)
        //   (W) add (1)   r2.0<1>:d -r3.0<0;1,0>:d  r3.2<0;1,0>:d  :small_start
        //   (W) add (1)   r70.0<1>:d  r2.0<0;1,0>:d  0x33333333:d  :small_patch
        //   (W) add (1)   r2.0<1>:d  -r3.0<0;1,0>:d r4.0<0;1,0>:d  :big_start
        //   (W) add (1)   r2.0<1>:d r2.0<0;1,0>:d  0x33333333:d    :big_patch
        //   if (BigEU)
        //    (W) mov (1)   r125.0<1>:f  r2.0<0;1,0>:f
        //        pseudo_fcall (16) r125.0<1>:ud r125.0<0;1,0>:ud  :big_call
        //   else
        //    (W) mov (1)            r125.0<1>:f r70.0<0;1,0>:f
        //        pseudo_fcall (16)  r125.0<1>:ud r125.0<0;1,0>:ud :small_call
        //
        //
        //  After finishFusedCallWA()
        //   (W) mov (1)              r2.0<1>:ud  sr0.0<0;1,0>:ud
        //   (W) and (16)  (eq)f1.0   null<1>:uw  r2.0<0;1,0>:uw  0x80:uw
        //   (W&!f1.0) mov (1)        cr0.2<1>:ud  r4.0<0;1,0>:ud
        //   (W) mov (1)           r3.2<1>:ud  cr0.2<0;1,0>:ud
        //
        //   (W) call (1)          r3.0<1>:d  _label_ip_wa
        //   _label_ip_wa:
        //   (W) add (1|M16)       r3.0<1>:d  r3.0<0;1,0>:d  0x20:d {NoCompact}
        //   (W) return (1)        r3.0<0;1,0>:d {NoCompact}
        //
        //   (W) add (1)           r2.0<1>:d  -r3.0<0;1,0>:d r3.2<0;1,0>:d  :IP
        //   (W) add (1)           r70.0<1>:d  r2.0<0;1,0>:d  144
        //   (W) add (1)           r2.0<1>:d  -r3.0<0;1,0>:d  r4.0<0;1,0>:d
        //   (W) add (1)           r2.0<1>:d  r2.0<0;1,0>:d   96
        //   if (BigEU)
        //    (W) mov (1)           r125.0<1>:f  r2.0<0;1,0>:f
        //        pseudo_fcall (16) r125.0<1>:ud  r125.0<0;1,0>:ud     : IP+96
        //   else
        //    (W) mov (1)           r125.0<1>:f  r70.0<0;1,0>:f
        //        pseudo_fcall (16) r125.0<1>:ud  r70.0<0;1,0>:f       : IP+144
        //
        // clang-format on
        BB->resetLocalIds();
        G4_INST *sI = callWAInfo.Small_start;
        G4_INST *bI = callWAInfo.Big_start;
        ip_inst = (sI->getLocalId() < bI->getLocalId() ? sI : bI);

        // Get IP to ip_inst.
        //   IP-WA's call sequence must be inserted right before ip_inst and
        //   IP must be stored in ip_wa's dst, not ip_inst's dst.
        InstListType waInsts;
        replaceIPWithCall(waInsts, ip_wa);

        // find IP adjustment add and set mask offset to M16!
        // (it is the 3rd inst!)
        G4_INST *adjust_ip_add = nullptr;
        for (auto tI : waInsts) {
          if (tI->opcode() == G4_add) {
            adjust_ip_add = tI;
            break;
          }
        }
        vASSERT(adjust_ip_add);
        kernel.setMaskOffset(adjust_ip_add, InstOpt_M16);

        auto ip_inst_ii = std::find(BB->begin(), BB->end(), ip_inst);
        BB->insert(ip_inst_ii, waInsts.begin(), waInsts.end());

        // Remove placeholder
        BB->remove(ip_wa);

        // finishFusedCallWA() will use this to calculate the offset.
        callWAInfo.IP_WA_placeholder = ip_inst;
      }
    }
  }
}

// Need to be done after SWSB so we can set call relative IP correctly.
void Optimizer::finishFusedCallWA() {
  // Regarding using M16 as maskOff to force running some instructions
  //
  // For each nested stack call like the following:
  //  (1) (W) mov (4|M0) r59.4<1>:ud r125.0<4;4,1>:ud  // save code in prolog
  //  (2) call (16|M0) r125.0     inner
  //  (3) (W) mov (4|M0) r125.0<1>:ud r59.4<4;4,1>:ud  // restore code in ret
  //  (4) ret  (16|M0)    r125.0
  // If no active channels,  call inst will always execute due to the hw bug,
  // therefore r125 will be modified by this call inst at (2). As no active
  // channels, r125 restore code at (3) is not going to be run. Therefore, r125
  // returned at (4) is not the one that is saved into r59.4 at (1), which is
  // wrong.
  //
  // The fix is to make save/restore mov instructions run always even though
  // there are no active channels.  They run if their quarter control is outside
  // the current JEU size (16 in this case), but still active (dmask still show
  // it is active). We will set dmask to simd32 in this case, quarter control to
  // M16 instead M0:
  //   (1) (W)  mov  (4|M16)    r59.4<1>:ud     r125.0<4;4,1>:ud
  //   (2)      call (16|M0)     r125.0          inner
  //   (3) (W)  mov  (4|M16)    r125.0<1>:ud    r59.4<4;4,1>:ud
  //
  // Note:
  //    r59.4 needs to write on stack frame before call and read back after call
  //    and its address payload needs to be correct. For this purpose, all call
  //    stack-related WA is done in RA, not here.
  //

  if (kernel.m_indirectCallWAInfo.empty() && kernel.m_maskOffWAInsts.empty())
    return;

  auto update_ip_distance = [](G4_INST *inst, int32_t &ip_dist) {
    G4_opcode op = inst->opcode();
    if (op == G4_sync_nop) {
      inst->setCompacted();
      ip_dist += 8;
    } else if (op != G4_label) {
      inst->setNoCompacted();
      ip_dist += 16;
    }
    return;
  };

  //    1. (W) mov (1|M0)            r2.0<1>:ud  sr0.0<0;1,0>:ud
  //    2. (W) and (16|M0) (eq)f1.0  null<1>:uw  r2.0<0;1,0>:uw    0x80:uw
  //    3. (W & ~f1.0) mov (1|M0)    cr0.2<1>:ud r3.0<0;1,0>:ud
  //    4. (W)mov (1|M0)             r64.0<1>:ud cr0.2<0;1,0>:ud
  // WA requires the mov at 4 to be in M16, not M0 in case the BigEU is off.
  // Here set quarter control of that mov to M16 (When stackcall is used,
  // only simd8/simd16 is allowed. Thus, we will set M16 always no matter
  // the kernel is simd8 or simd16).
  for (const auto& II : kernel.m_maskOffWAInsts) {
    G4_INST *tInst = II.first;
    kernel.setMaskOffset(tInst, InstOpt_M16);
  }

  // indirect relative call
  for (const auto &II : kernel.m_indirectCallWAInfo) {
    G4_BB *BB = II.first;
    const IndirectCallWAInfo &callWAInfo = II.second;

    if (callWAInfo.Small_start == nullptr) { // calla, skip
      continue;
    }

    // finishFusedCallWA_preSWSB() sets this placeholder.
    G4_INST *ip_inst = callWAInfo.IP_WA_placeholder;

    // IP WA is applied if ip_inst isn't null.
    for (int i = 0; i < 2; ++i) {
      G4_INST *patch_add =
          (i == 0 ? callWAInfo.Small_patch : callWAInfo.Big_patch);
      G4_INST *ip_start =
          (i == 0 ? callWAInfo.Small_start : callWAInfo.Big_start);
      if (ip_inst) {
        // IP WA: ip is taken at ip_inst for both small and big targets.
        ip_start = ip_inst;
      }
      G4_INST *ip_end = (i == 0 ? callWAInfo.Small_call : callWAInfo.Big_call);
      G4_BB *start_bb = BB;
      G4_BB *end_bb = (i == 0 ? callWAInfo.Small_BB : callWAInfo.Big_BB);

      int32_t dist = 0;
      G4_BB *b;
      G4_BB *next_b = start_bb;
      INST_LIST_ITER it_start =
          std::find(start_bb->begin(), start_bb->end(), ip_start);
      INST_LIST_ITER it_end = std::find(end_bb->begin(), end_bb->end(), ip_end);
      do {
        b = next_b;
        INST_LIST_ITER iter = (b == start_bb ? it_start : b->begin());
        INST_LIST_ITER iterEnd = (b == end_bb ? it_end : b->end());
        for (; iter != iterEnd; ++iter) {
          G4_INST *tI = *iter;
          update_ip_distance(tI, dist);
        }
        next_b = b->getPhysicalSucc();
      } while (b != end_bb && next_b != nullptr);
      vASSERT(b == end_bb);

      G4_Imm *distOprd = builder.createImm(-dist, Type_D);
      patch_add->setSrc(distOprd, 1);
    }
  }

  // RA does the following
  //  (W) mov(1|M0)  r125.0<1>:f   r60.0<0;1,0>:f
  //  (W) send.dc0(16|M0)  null r126  r5  0x80  0x020A03FF   // stack spill
  //      sync.nop        null{ Compacted,$4.src }
  //      call (8|M0)      r125.0   r125.0
  //
  // To make call WA work,  call for SmallEU has to use r60, not r125, as below:
  //   call (8|M0)  r125.0 r60.0
  // Here propogate r60.0 down to call instruction
  // (For call, can just copy patch's dst to call's target. Here the code works
  //  for both call and calla.)
  for (const auto &II : kernel.m_indirectCallWAInfo) {
    const IndirectCallWAInfo &callWAInfo = II.second;

    G4_INST *iCallInst = callWAInfo.Small_call;
    G4_BB *B = callWAInfo.Small_BB;
    vASSERT(iCallInst->isFCall() && iCallInst->getSrc(0)->isGreg());

    bool isValid;
    G4_SrcRegRegion *T = iCallInst->getSrc(0)->asSrcRegRegion();
    int regno = T->ExRegNum(isValid);
    int subreg = T->ExSubRegNum(isValid);

    // Search backward to find the the 1st mov that defined this reg
    // This works for ifcall that has been put into a separate BB, in
    // which only insts related to call sequence are present in the BB.
    // If not found, do nothing.
    INST_LIST_ITER it_end = std::find(B->begin(), B->end(), iCallInst);
    vASSERT(it_end != B->end());
    for (auto II = it_end, IB = B->begin(); II != IB; --II) {
      auto prevII = std::prev(II);
      G4_INST *tInst = *prevII;
      if (tInst->opcode() == G4_mov && tInst->getExecSize() == g4::SIMD1 &&
          tInst->isWriteEnableInst() && tInst->getDst()->isGreg() &&
          tInst->getSrc(0)->isGreg() &&
          T->getTypeSize() == tInst->getSrc(0)->getTypeSize()) {
        G4_DstRegRegion *D = tInst->getDst();
        int dst_regno = D->ExRegNum(isValid);
        int dst_subreg = D->ExSubRegNum(isValid);
        if (dst_regno == regno && subreg == dst_subreg) {
          // found
          G4_SrcRegRegion *Src0 = tInst->getSrc(0)->asSrcRegRegion();
          G4_SrcRegRegion *newT = builder.createSrcRegRegion(*Src0);
          iCallInst->setSrc(newT, 0);
          break;
        }
      }
    }
  }

  kernel.m_maskOffWAInsts.clear();
  kernel.m_indirectCallWAInfo.clear();
}

void Optimizer::adjustIndirectCallOffsetAfterSWSBSet() {
  // the call code sequence done at Optimizer::expandIndirectCallWithRegTarget
  // is:

  // if has IP WA, more instructions are added:
  //     call   dst     _label_ip_wa
  //   _label_ip_wa:
  //     add    dst     dst     32     // 3rd add, sync_off_2
  //                                   // 32 is hardcoded
  //     ret    dst
  // else it'll be :
  //     add  r2.0  -IP   call_target  // 2nd add
  //     add  r2.0  r2.0  -32          // 1st add, sync_off_1
  //                                   // -32 is hardcoded
  //     call r1.0  r2.0
  // SWSB could've inserted sync instructions between offset-hardcoded
  // instructions. We need to re-adjust the offset

  // update the offset if the given inst is a sync
  // return true if inst is sync
  auto update_sync_off = [](G4_INST &inst, uint64_t &sync_offset) {
    G4_opcode op = inst.opcode();
    if (op == G4_sync_allrd || op == G4_sync_allwr) {
      inst.setNoCompacted();
      sync_offset += 16;
      return true;
    } else if (op == G4_sync_nop) {
      inst.setCompacted();
      sync_offset += 8;
      return true;
    }
    return false;
  };

  for (auto bb : kernel.fg) {
    if (bb->empty())
      continue;

    if (bb->back()->isFCall()) {
      G4_InstCF *fcall = bb->back()->asCFInst();
      if (fcall->isIndirectCall()) {
        // for every indirect call, count # of instructions inserted
        // between call and the first add
        uint64_t sync_off_1 = 0;
        G4_INST *first_add = nullptr;
        INST_LIST::reverse_iterator it = bb->rbegin();
        // skip call itself
        ++it;
        // calculate sync_off_1
        for (; it != bb->rend(); ++it) {
          G4_INST &inst = **it;
          if (update_sync_off(inst, sync_off_1))
            continue;
          else if (inst.opcode() == G4_add) {
            if (first_add == nullptr) {
              first_add = &inst;
              continue;
            } else {
              // found 2nd add
              break;
            }
          }
          // instructions between pattern sequence could only be
          // sync.nop, sync.allrd or sync.allwr
          vASSERT(false);
        }
        vASSERT(first_add->getSrc(1)->isImm());
        int64_t adjust_off =
            first_add->getSrc(1)->asImm()->getInt() - sync_off_1;
        first_add->setSrc(builder.createImm(adjust_off, Type_D), 1);

        // calculate sync_off_2
        if (builder.needIPWA()) {
          // at this point, it should point to 2nd add, skip it
          ++it;
          uint64_t sync_off_2 = 0;
          G4_INST *third_add = nullptr;
          for (; it != bb->rend(); ++it) {
            G4_INST &inst = **it;
            if (update_sync_off(inst, sync_off_2))
              continue;
            else if (inst.opcode() == G4_return)
              continue;
            else if (inst.opcode() == G4_add) {
              vASSERT(third_add == nullptr);
              third_add = &inst;
              break;
            }
            // instructions between pattern sequence could only be
            // sync.nop, sync.allrd or sync.allwr
            vASSERT(false);
          }
          vASSERT(third_add->getSrc(1)->isImm());
          int64_t adjust_off_2 =
              third_add->getSrc(1)->asImm()->getInt() + sync_off_2;
          third_add->setSrc(
              builder.createImm(adjust_off_2, third_add->getSrc(1)->getType()),
              1);
        }
      }
    }
  }
}

// [NoMask WA]
// EU Fusion introduced a new hardware : fused Mask (2 bits, one for each fused
// EUs to indicate whether EU is on or off) to control NoMask instructions from
// running on off EU. However, there is a hw bug that will not let the fused
// mask change from 01 to 00, causing off EU to run NoMask inst that should not
// run.
//
// A WA is to change any NoMask instruction by adding a predicate to it.
// And this predicate is equivalent to correct NoMask semantics. For example,
// the following instruction
//
//    (W) add (8|M0)  r10.0<1>:d  r11.0<1;1,0>:d  r12.0<1;1,0>:d
//
//  will be changed to
//
//    (W)  mov (1|M0) f0.0<1>:w  0
//         cmp (8|M0) (eq)f0.0 r0:uw  r0:uw
//    (W&f0.0.any8h) add (8|M0)  r10.0<1>:d  r11.0<1;1,0>:d  r12.0<1;1,0>:d
//
//  Note that f0.0 is called "WA flag".
//
// The HW still have the correct CE mask so that the above mov&cmp sequence
// still works, that is, f0.0 will be all zero if no active lanes and will not
// be zero if there is at least one active lane.
//
// Nested Divergence
//   For a fused mask to be 01,  the control-flow must be divergent
//   at that point. Furthermore, changing 01 to 00 happens only if a further
//   divergence happens within a already-divergent path. This further
//   divergence is referred to as the nested divergence.
//
//   As changing from 01 to 00 never happens with backward goto, backward
//   goto is treated as divergent, but not nested divergent for the purpose
//   of this WA.
//
// This function first finds out which BB are in nested divergent branch and
// then add predicates to those NoMask instructions.
//
// [Some details]
// --------------
// This WA could be understood in terms of physical registers. When a NoMask
// instruction runs when it should not, it will change physical registers. If
// the physical registers have valid values that will be used later, this NoMask
// instruction will result in incorrect values in those registers.  Here is an
// example:
// clang-format off
//                                                       fusedMask
//        (0)  (f0.0.any16h) goto(16)  BB1                  [11]
//  BB0                                                     [01]
//        (1)  (W) mov (1|M0)  f0.1<1>:uw   0x3:uw
//        (2)      goto BB3
//
//   BB1:                                                   [01, should be 00]
//        (3)      join (16)                                [11, should be 10]
//        (4)  (W) mov (1|M0)  f0.1<1>:uw   0x0:uw
//        (5)      cmp (16|M0) (eq)f0.1  null<1>:uw  r0.0<0;1,0>:uw r0.0<0;1,0>:uw
//        (6)  (W&f0.1.any16h) mov (1|M0)  f0.1<1>:uw   0x0:uw
//   BB2:                                                   [11, should be 10]
//        (7)  or (8|M0) (ne)f0.1  null<1>:uw r1.4<8;8,1>:uw  r3.0<8;8,1>:uw
//
//   BB3:
//        (8)         join (16)                             [11, correct]
//        (9)  (f0.1) sel (8|M0)  r1.4<1>:uw   r1.3<0;1,0>:uw  0x0:uw
// clang-format on
//
//  where (4) & (5) are WA instructions. (6) has WA applied. f0.1 at (9) takes
//  value either defined at (1) or (7).  Suppose BigEU takes BB0 and SmallEU
//  takes BB1-BB2 and both BigEU and SmallEU will join at (8). Thus, (9) of
//  BigEU will take its value defined at (1) in BB0. Due to this HW bug, BigEU
//  will execute noMask instruction (4) in BB1, causing f0.1's value to be
//  changed. As a result, (9) of BigEU will actually take the value defined at
//  (4), which is wrong.
//
//  To prevent this from happening, the workaround flag will have the following
//  sequence:
//             (W) mov (1|M0)  r32.3:uw  f0.1         // save f0.1
//        (4)  (W) mov (1|M0)  f0.1<1>:uw   0x0:uw
//        (5)      cmp (16|M0) (eq)f0.1  null<1>:uw  r0.0<0;1,0>:uw
//        r0.0<0;1,0>:uw (6)  (W&f0.1.any16h) mov (1|M0)  f0.1<1>:uw   0x0:uw
//             (W) mov (1|M0)  f0.1 f32.3:uw          // restore f0.1
//  In doing so, f0.1 will be the original value, and the above issue is
//  avoided.
//
//  Since new mov (save/restore f0.1) instructions are noMask instructions,
//  r32.3 is also needed to avoid clobbering any valid variables allocated to
//  r32.3 too.
//
//  We guarantee this by reserving GRFs as needed during applying WAs.
//
// [more on insts after register allocation]
// -----------------------------------------
//   Assuming BB1 is on off EU.
//
//   V77 (2GRF) spills at offset[4x32]. The following code reads V77 from spill
//   location, and modifies it, and finally write the result back into
//   offset[4xi32]. If the code can keep the content at this location unchanged,
//   no WA is needed; otherwise, we must have WA.
//
//   But write at (3) will write whatever in r4 into offset[4x32],  which is
//   undefined, definitely not guaranteed to be the same as r1 just read from
//   the same location. (Note that mul at (2) will not run because the channel
//   enable is off. Thus it modifies the content at offset[4x32], which is
//   wrong.
//
//   Before RA:
//     BB1:
//       mul (M1, 16) V77(0,0)<1> V141(0,0)<0;1,0> V77(0,0)<1;1,0>
//     BB2:
//       svm_block_st (4) V154(0,0)<0;1,0> V77.0
//
//   After RA
//     BB1:
//      (1)  // wr:1h+0, rd:2; hword scratch block read x2
//           // scratch space fill: FL_GRF_V77_6 from offset[4x32]
//           (W) send.dc0 (16|M0)  r1  r0  null  0x0  0x022C1004
//      (2)  mul (16|M0)  r4.0<1>:f  r3.0<0;1,0>:f  r1.0<8;8,1>:f
//      (3)  // wr:1h+2, rd:0; hword scratch block write x2
//           //  scratch space spill: SP_GRF_V77_3 from offset[4x32];
//           (W) send.dc0 (16|M0)  null  r0  r4  0x80  0x020F1004
//
// For flag spill:
//   Need WA as well due to the following case:
//
//   After RA:
//      BB_19:
//           (W)  mov (1|M0)     r34.8<1>:uw   f0.1<0;1,0>:uw
//           ...
//      BB_21:
//           (W)  mov (1|M0)     f1.1<1>:uw    r34.8<0;1,0>:uw
//
//   If BB_19 should be skipped but runs due to this HW bug, r34.8 will be
//   updated with a f0.1, which is undefined value.  And at BB_21, reading from
//   r34.8 will get garbage value!
// ======================================================================================
// The NoMask WA has two parts:
//        preRA part: prepare for applying WA in postRA
//       postRA part: apply WAs
//
// prepareNoMaskWA is preRA part. It does:
//    1. Determines if NoMask WA needs to be applied for any BB
//       This is done by using nested divergence to decide whether a BB needs
//       WA.
//    2. If WA is needed,  reserve dedicated GRFs
//       Check all insts that need WA and decide how much GRF to be reserved.
//       At most 2GRF + 2DW is needed.
//    This info, reserved GRFs and whether there are insts that need WA, is
//    passed into postRA.  Note that even though there is no inst that need WA
//    preRA, it is still possible that spill/fill needs WA. Thus, at least 2DW
//    will be reserved.
//
// ApplyNoMaskWA() : postRA part.
void Optimizer::prepareNoMaskWA() {
  std::unordered_map<G4_BB *, int> nestedDivergentBBs;
  const G4_ExecSize simdsize = fg.getKernel()->getSimdSize();

  // Identify BBs that need WA
  fg.reassignBlockIDs();
  fg.findNestedDivergentBBs(nestedDivergentBBs);

  // Return true if a NoMask inst is either send or global
  auto isCandidateInst = [&](G4_INST *Inst, FlowGraph &cfg) -> bool {
    // pseudo should be gone at this time [skip all pseudo].
    if (!Inst->isWriteEnableInst() || Inst->isCFInst() ||
        Inst->isPseudoLogic() || Inst->isPseudoKill() ||
        Inst->isWait() ||         // predicate not supported
        Inst->opcode() == G4_nop) // predicate not supported
    {
      return false;
    }
    if (Inst->isSend() && Inst->getPredicate() &&
        Inst->getExecSize() > simdsize) {
      // fused send, already correctly predicated, skip
      return false;
    }
    if (Inst->isEOT()) {
      // Algo assumes no WA needed for entry and exit, skip EOT for now.
      return false;
    }
    return true;
  };

  // If true, there exist NoMask insts that need WA.
  bool hasWAInst = false;
  bool reserveWAFlag = false;
  uint32_t numTempInUD = 0; // size of temp in UD
  G4_SubReg_Align tempAlign = Even_Word;

  auto updateTempReserve = [&](uint32_t aNumElts, G4_Type aEltTy,
                               G4_SubReg_Align aAlign) {
    uint32_t newBytes = aNumElts * TypeSize(aEltTy);
    uint32_t newDWs = (newBytes + 3) / 4;
    if (newDWs > numTempInUD) {
      numTempInUD = newDWs;
    }
    if (tempAlign < aAlign) {
      tempAlign = aAlign;
    }
  };

  // Scan all insts and mark then if WAs are needed
  for (auto BI : fg) {
    G4_BB *BB = BI;
    if ((BB->getBBType() & G4_BB_NM_WA_TYPE) == 0) {
      continue;
    }

    // This BB might need WA, thus reserved GRF for WA flags.
    // (Even though there is no NoMask inst in this BB now, later RA might
    // generate
    //  spill/fill in this BB. Thus WAFlagReserve shoud be set here.)
    reserveWAFlag = true;
    for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II) {
      G4_INST *I = *II;
      if (isCandidateInst(I, fg)) {
        I->setNeedPostRA(true);
        hasWAInst = true;

        // Check if any temps are needed.
        G4_CondMod *condmod = I->getCondMod();
        G4_Predicate *pred = I->getPredicate();
        if (I->opcode() == G4_sel || I->opcode() == G4_csel) {
          // doFlagModifierSelInstWA : temp for saving dst (could be 2GRF)
          //   Note: sel's pred isn't used for calculating WrEn, and csel does
          //         not allow predicate.
          G4_DstRegRegion* dst = I->getDst();
          if (dst && !dst->isNullReg()) {
            (void)updateTempReserve(I->getExecSize() * dst->getHorzStride(),
              dst->getType(), dst->getTopDcl()->getSubRegAlign());
          }
          else
            vISA_ASSERT(false, "ICE: expect dst to be non-null!");
        } else if (pred && !condmod) {
          // doPredicateInstWA(): need 1 DW
          updateTempReserve(1, Type_UD, Even_Word);
        } else if (!pred && condmod) {
          // doFlagModifierInstWA : temp for saving condmod
          updateTempReserve(1, Type_UD, Even_Word);
        } else if (pred && condmod) {
          // doPredicateAndFlagModifierInstWA : temp for saving predicate
          updateTempReserve(1, Type_UD, Even_Word);
        }
      }
    }
  }

  G4_BB *entryBB = fg.getEntryBB();
  vASSERT(entryBB);
  G4_Declare *WATemp = nullptr;
  if (numTempInUD > 0) {
    // For temps other than WA flags. Its size will be the largest of all temps
    // It is at most 2 GRF (dst that uses maximum 2 GRF).
    WATemp = builder.createTempVar(numTempInUD, Type_UD, tempAlign, "WATemp");
    WATemp->setLiveIn();
    WATemp->setLiveOut();
    WATemp->setDoNotSpill();

    // Add a pseudo use inst so that RA will include this temp for reg
    // allocation.
    G4_ExecSize sz =
        builder.toExecSize(Get_VISA_Exec_Size_From_Raw_Size(numTempInUD));
    G4_SrcRegRegion *use =
        builder.createSrc(WATemp->getRegVar(), 0, 0,
                          (sz == g4::SIMD1 ? builder.getRegionScalar()
                                           : builder.getRegionStride1()),
                          Type_UD);
    G4_INST *pseudoUseInst = builder.createIntrinsicInst(
        nullptr, Intrinsic::FlagSpill, sz, nullptr, use, nullptr, nullptr,
        InstOpt_NoOpt, false);

    INST_LIST_ITER inst_it = entryBB->getFirstInsertPos();
    entryBB->insertBefore(inst_it, pseudoUseInst);
  }

  // WA flag temp:  2 DW.
  //    The First for saving the existing flag so that WA flag can use it.
  //    The second one is a temp for saving WA flag to avoid recalculating it.
  G4_Declare *WAFlagReserve = nullptr;
  if (reserveWAFlag) {
    WAFlagReserve = builder.createTempVar(2, Type_UD, Even_Word, "WAFlag");
    WAFlagReserve->setLiveIn();
    WAFlagReserve->setLiveOut();
    WAFlagReserve->setDoNotSpill();

    G4_SrcRegRegion *src = builder.createSrc(
        WAFlagReserve->getRegVar(), 0, 0, builder.getRegionStride1(), Type_UD);
    G4_INST *pseudoUseInst = builder.createIntrinsicInst(
        nullptr, Intrinsic::FlagSpill, g4::SIMD2, nullptr, src, nullptr,
        nullptr, InstOpt_NoOpt, false);

    INST_LIST_ITER inst_it = entryBB->getFirstInsertPos();
    entryBB->insertBefore(inst_it, pseudoUseInst);
  };

  // Save info for applyNoMaskWA() to use after RA.
  // If reserveWAFlag is false, there is no need to apply WA at all (including
  // postRA).
  if (reserveWAFlag) {
    kernel.createNoMaskWAInfo(WAFlagReserve, WATemp, hasWAInst);
  }
}

void Optimizer::applyNoMaskWA() {
  // Utility class to get flag def/use info for a BB
  //    Each of 16-bit flag has one bit to track whether it is used or defined.
  //    We have 4 flags, thus 4 bits for use and 4 bits for def.
  //
  //    DefUse info is encoded as uint32_t, in which the first 4 bits of 1st
  //    half and the 2nd half are for use and def, respectively, that is,
  //        [3:0] : use (f1.1, f1.0, f0.1, f0.0)
  //      [19:16] : def (f1.1, f1.0, f0.1, f0.0)
  //
  // For example,  0xA0001 (1010b, 0001b) -> f1.1 & f0.1 are defined, f0.0 is
  // used
  //
  // Convention:
  //    Inst iterator range is represented as [a, b], or [a, b), in which '['
  //    and ']' means inclusive, where '(' and ')' means exclusive.  For
  //    example, [1, 10) means 1 to 9, where [1, 10] means 1 to 10.
  class FlagDefUse {
    G4_BB *m_BB;
    // Keep track DefUse info for each inst.
    std::unordered_map<G4_INST *, uint32_t> m_flagDefUse;

  public:
    FlagDefUse(G4_BB *aBB) : m_BB(aBB) {}

    // return value:
    //   true:  if "O" is flag and has assigned a physical flag. This physical
    //   reg
    //          is returned as (freg, fsreg):ty.
    //   false: otherwise
    //
    // Note this code mimics the logic of printRegVarOff() in G4_IR.cpp.
    //
    // For pred/condMod, "ty" is the actual size that this "O" accesses,
    // not the decl size of "O". For example,
    //    cmp  (16|M16)  (eq)f0.0  ...
    // this func returns with f(0,0):UW, but "O" is of UD!
    static bool getFlagRegAndSubreg(G4_Operand *O, uint32_t &freg,
                                    uint32_t &fsreg, G4_Type &ty) {
      // flag:
      //        reg no = base's ExRegNum()
      //     subregoff = base's subregoff + Operand's subregoff  (in UW)
      //
      // Type difference b/w base and operand is not considered here for flag as
      // the base's type is always UW. Operand's type can be UW/UD. If operand's
      // type is UD, its subregoff in UD must be 0, which is the same as one in
      // UW. Therefore, simply treat operand's subRegOff as in UW.
      uint32_t nSubFlag = (O->getRightBound() - O->getLeftBound() + 16) / 16;
      uint32_t subregoff = 0;
      if (O->isSrcRegRegion()) {
        subregoff = O->asSrcRegRegion()->getSubRegOff();
      } else if (O->isDstRegRegion()) {
        subregoff = O->asDstRegRegion()->getSubRegOff();
      } else if (O->isPredicate()) {
        subregoff = O->asPredicate()->getSubRegOff();
      } else if (O->isCondMod()) {
        subregoff = O->asCondMod()->getSubRegOff();
      }

      G4_VarBase *BVar = O->getBase();
      ty = (nSubFlag == 1 ? Type_UW : Type_UD);
      bool isValid = false;
      if (BVar) {
        freg = BVar->ExRegNum(isValid);
        fsreg = BVar->asRegVar()->getPhyRegOff() + subregoff;
      }
      return isValid;
    }

  private:
    uint16_t getFlagBits(G4_Operand *O) {
      uint32_t r, sr;
      G4_Type t;
      if (getFlagRegAndSubreg(O, r, sr, t)) {
        // For the following cases, getFlagRegAndSubreg() returns with r=1,
        // sr=0, ty=UW. But they really access f1.1. Thus, do adjustment to get
        // the right flag bits!
        //          cmp (16|M16) (eq)f1.0 ...
        //   (f1.0) mov (16|M16) ....
        if ((O->isPredicate() || O->isCondMod()) && t == Type_UW) {
          // sanity check: subreg could be 1 only if rightBound < 16
          vASSERT(sr == 0 || O->getRightBound() < 16);

          if (O->getLeftBound() >= 16) {
            // typical cases like ones in comments above
            sr = 1;
          } else if (O->getRightBound() >= 16) {
            // cross two sub-flags (f1.0 and f1.1). Reset t to UD
            t = Type_UD;
          }
        }

        uint16_t bits = (t == Type_UD ? 0x3 : 0x1);
        return (bits << (r * 2 + sr));
      }
      vISA_ASSERT_UNREACHABLE("Flag: not allocated to physical register!");
      return 0;
    };

    uint32_t getFlagDefUseBits(G4_INST *aI) {
      auto MI = m_flagDefUse.find(aI);
      if (MI != m_flagDefUse.end()) {
        return MI->second;
      }

      uint16_t flagUse = 0;
      uint16_t flagDef = 0;
      for (int i = 0, sz = (int)aI->getNumSrc(); i < sz; ++i) {
        G4_Operand *S = aI->getOperand(aI->getSrcOperandNum(i));
        if (S && S->isFlag()) {
          vASSERT(S->asSrcRegRegion()->getBase()->getAreg());
          flagUse |= getFlagBits(S);
        }
      }
      // predicate
      if (G4_Predicate *P = aI->getPredicate()) {
        flagUse |= getFlagBits(P);
      }
      // defs
      G4_Operand *D = aI->getDst();
      if (D && !D->isNullReg() && D->isFlag()) {
        vASSERT(D->asDstRegRegion()->getBase()->getAreg());
        flagDef |= getFlagBits(D);
      }
      if (aI->opcode() != G4_sel &&
          aI->opcode() != G4_csel) { // sel does not update condMod
        if (G4_CondMod *Mod = aI->getCondMod()) {
          flagDef |= getFlagBits(Mod);
        }
      }
      uint32_t retBits = (flagDef << 16) | flagUse;
      m_flagDefUse.insert(std::make_pair(aI, retBits));
      return retBits;
    }

    // Return flag bits for instructions within [SI, EI).
    uint32_t getInstsBits(INST_LIST_ITER SI, INST_LIST_ITER EI) {
      uint32_t defuse = 0;
      for (auto II = SI; II != EI; ++II) {
        G4_INST *tI = *II;
        defuse |= getFlagDefUseBits(tI);
      }
      return defuse;
    }

    // Return  true: if there is a flag that is not referenced by this duBits.
    //               The returned flag (freg, fsreg) is a unreferenced one.
    //        false: otherwise.
    bool getUnreferencedFlag(uint32_t duBits, G4_Type fty, uint32_t &freg,
                             uint32_t &fsreg) {
      uint32_t fBits = (fty == Type_UD) ? 0x3 : 0x1;
      uint32_t duBitsD = (duBits >> 16);
      int i = 0;
      for (; i < 4; i += (fty == Type_UD ? 2 : 1)) {
        if ((fBits & duBits) == 0      // Use
            && (fBits & duBitsD) == 0) // Def
        {
          freg = i / 2;
          fsreg = i % 2;
          return true;
        }
        fBits = (fBits << (fty == Type_UD ? 2 : 1));
      }
      return false;
    }

  public:
    // Let BI = aWaInsts[aStartIx], EI = ++(aWaInsts.back()).
    // Note that aWaInsts's element is of INST_LIST_ITER.
    //
    // getBestFlagIfAvailable() searches [BI, EI), and it searches in order
    // until no available flag can be used. (In doing so, we have the maximum
    // number of WA insts that can use the same WA flag.) The argument 'aEndIx'
    // is the index it stops when no flag can be used.
    //   Return value:
    //     false:  If aEndIx == aStartIx,  no flag can be used. This means that
    //     the inst at aStartIx takes
    //             all two flags.
    //      true:  otherwise, (retFreg, retFsreg):FTy is not used in [
    //      aWaInsts[aStartIx], aWaInsts[aEndIx] ).
    //             If aEndIx = aWaInsts.size(), it means (retFreg, retFsreg):FTy
    //             can be used for all insts of aWaInsts, starting from
    //             aStartIx.
    bool getBestFlagIfAvailable(const std::vector<INST_LIST_ITER> &aWaInsts,
                                const int32_t aStartIx, int32_t &aEndIx,
                                G4_Type FTy, uint32_t &retFreg,
                                uint32_t &retFsreg) {
      // initialize flag to be invalid
      retFreg = 0xff;
      retFsreg = 0xff;

      int SIx = aStartIx;
      INST_LIST_ITER BI = aWaInsts[SIx];
      uint32_t DUBits = 0;
      for (const int EIx = (int)aWaInsts.size(); SIx < EIx; ++SIx) {
        uint32_t r, s;
        INST_LIST_ITER NI = std::next(aWaInsts[SIx]);
        DUBits |= getInstsBits(BI, NI);
        if (!getUnreferencedFlag(DUBits, FTy, r, s)) {
          // no flag is available at ix
          break;
        }
        retFreg = r;
        retFsreg = s;
        BI = NI; // set the next starting iterator
      }

      aEndIx = SIx;
      return SIx != aStartIx;
    }
  };

  // Only need to create at most 6 WAFlag temps.
  G4_Declare *FlagUD[2] = {nullptr, nullptr};
  G4_Declare *FlagUW[4] = {nullptr, nullptr, nullptr, nullptr};
  auto getFlagDcl = [&](uint32_t aFreg, uint32_t aFsreg, G4_Type aFTy) {
    G4_Declare *retDcl;
    if (aFTy == Type_UD) {
      int ix = aFreg;
      vASSERT(ix < ARRAY_COUNT(FlagUD));
      if (FlagUD[ix] == nullptr) {
        FlagUD[ix] = builder.createTempFlag(2, "WAFlagUD");
      }
      retDcl = FlagUD[ix];
    } else {
      int ix = 2 * aFreg + aFsreg;
      vASSERT(ix < ARRAY_COUNT(FlagUW));
      if (FlagUW[ix] == nullptr) {
        FlagUW[ix] = builder.createTempFlag(1, "WAFlagUW");
      }
      retDcl = FlagUW[ix];
    }
    return retDcl;
  };

  // Get those GRFs reserved in prepareNoMaskWA()
  NoMaskWAInfo *WAInfo = kernel.getEUFusionNoMaskWAInfo();

  // If no spill AND no inst that needs WA, just return.
  //   ' HasWAInsts = true' means that before RA, there are insts that need WA
  const bool HasFlagSpill = (builder.getJitInfo()->stats.numFlagSpillStore > 0);
  const bool HasGRFSpill = (builder.getJitInfo()->stats.spillMemUsed > 0);
  if (!WAInfo || // No BB needs WA
      (!(HasFlagSpill || HasGRFSpill) &&
       !WAInfo->HasWAInsts)) // No Spill, no WA Insts
  {
    kernel.deleteEUFusionNoMaskWAInfo();
    return;
  }

  const G4_ExecSize Simdsize = fg.getKernel()->getSimdSize();
  const RegionDesc *ScalarReg = builder.getRegionScalar();
  bool UseAnyh = true; // default, adjusted for each BB.

  // WAFlagReserve is 2DW GRF.
  // An example about how to use it.
  //     Assume WAFlag is f0.1:uw
  //
  //   ===========================================
  //   |          DW0        |        DW         |
  //   |   uw0     |   uw1   |   uw0   |   uw1   |
  //   ===========================================
  //   | orig f0.1 |         | WA f0.1 |         |       <-- WAFlag = f0.1:uw
  //   ============================================
  //   |      orig  f0.0     |    WA f0.0        |       <-- WAFlag = f0.0:ud
  //   ===========================================
  //
  // If WAFlag cannot be used to all insts as it is clobbered somewhere in the
  // middle, it must be saved in DW1.
  //
  G4_Declare *SaveDcl = WAInfo->WAFlagReserved; // 2DW
  G4_RegVar *SaveVar = SaveDcl->getRegVar();
  G4_Declare *WATempDcl = WAInfo->WATempReserved; // 0 - 2 GRF
  G4_RegVar *WATempVar = (WATempDcl ? WATempDcl->getRegVar() : nullptr);

#if defined(_DEBUG) || defined(_INTERNAL)
  // Check if linearStart has been done and SaveDcl/WATempDcl has been
  // allocated. (computePReg() set GRFBaseOffset().
  auto checkDclPReg = [&](G4_Declare *aDcl) {
    // Set lineartStar for aDcl
    G4_RegVar *RegVar = aDcl->getRegVar();
    vASSERT(RegVar->isPhyRegAssigned() && RegVar->getPhyReg()->isGreg());
    uint32_t regNum =
        (static_cast<G4_Greg *>(RegVar->getPhyReg()))->getRegNum();
    uint32_t subRegNum = RegVar->getPhyRegOff();
    uint32_t dclEltBytes = aDcl->getElemSize();
    uint32_t linearizedStart =
        (regNum * builder.numEltPerGRF<Type_UB>()) + (subRegNum * dclEltBytes);
    vASSERT(aDcl->getGRFOffsetFromR0() == linearizedStart);
  };

  checkDclPReg(SaveDcl);
  if (WATempDcl != nullptr) {
    checkDclPReg(WATempDcl);
  }
#endif

  auto verifyRegVarSize = [&](G4_RegVar *aRegVar, uint32_t aBytes) {
#if defined(_DEBUG) || defined(_INTERNAL)
    uint32_t var_sz =
        (aRegVar != nullptr ? aRegVar->getDeclare()->getByteSize() : 0);
    if (var_sz < aBytes) {
      vISA_ASSERT(false, "WATemp does not reserve enough space!");
    }
#endif
  };

  auto WAFlagSaveOff = [](G4_Type aT) { return aT == Type_UD ? 1 : 2; };
  auto isNull = [](G4_Operand *aO) {
    return (aO == nullptr || aO->isNullReg());
  };

  auto getPredCtrl = [&Simdsize](bool aUseAnyh) -> G4_Predicate_Control {
    if (aUseAnyh) {
      return Simdsize == g4::SIMD8
                 ? PRED_ANY8H
                 : (Simdsize == g4::SIMD16 ? PRED_ANY16H : PRED_ANY32H);
    }
    return PRED_DEFAULT;
  };

  auto isCandidate = [](G4_INST *I) {
    return (I->getNeedPostRA() && I->isWriteEnableInst());
  };

  // Create WAFlag using mov and cmp.
  auto createFlagFromCmp = [&](G4_BB *aBB, INST_LIST_ITER &aInsertBeforePos,
                               G4_RegVar *aFlag, G4_Type aTy) {
    //  I0:     (W) mov (1|M0)  f0.0<1>:aTy,  0
    //  I1:         cmp (Simdsize|M0) (eq)f0.0  r0<0;1,0>:uw  r0<0;1,0>:uw
    //  I2      (W&f0.0.anyh) mov (1|M0) f0.0:aTy   0xffffffff:aTy [optional]
    G4_DstRegRegion *D = builder.createDst(aFlag, 0, 0, 1, aTy);
    G4_INST *I0 = builder.createMov(g4::SIMD1, D, builder.createImm(0, aTy),
                                    InstOpt_WriteEnable, false);
    aBB->insertBefore(aInsertBeforePos, I0);

    G4_RegVar *cmpVar;
    const bool USE_R0_FOR_EMASK_CMP = false;
    if (USE_R0_FOR_EMASK_CMP) {
      cmpVar = builder.getRealR0()->getRegVar();
    } else {
      // using r2.0:uw for cmp
      G4_Declare *cmpDcl = builder.createHardwiredDeclare(1, Type_UW, 2, 0);
      cmpVar = cmpDcl->getRegVar();
    }

    G4_SrcRegRegion *r_0 = builder.createSrc(cmpVar, 0, 0, ScalarReg, Type_UW);
    G4_SrcRegRegion *r_1 = builder.createSrc(cmpVar, 0, 0, ScalarReg, Type_UW);
    G4_CondMod *flagCM = builder.createCondMod(Mod_e, aFlag, 0);
    G4_DstRegRegion *nullDst = builder.createNullDst(Type_UW);
    G4_INST *I1 =
        builder.createInternalInst(NULL, G4_cmp, flagCM, g4::NOSAT, Simdsize,
                                   nullDst, r_0, r_1, InstOpt_M0);
    aBB->insertBefore(aInsertBeforePos, I1);

    if (!UseAnyh) {
      G4_Imm *allone = builder.createImm(0xFFFFFFFF, aTy);
      G4_DstRegRegion *tF = builder.createDst(aFlag, 0, 0, 1, aTy);
      G4_INST *I2 =
          builder.createMov(g4::SIMD1, tF, allone, InstOpt_WriteEnable, false);
      G4_Predicate *I2_P = builder.createPredicate(
          PredState_Plus, aFlag, 0,
          (Simdsize == g4::SIMD8
               ? PRED_ANY8H
               : (Simdsize == g4::SIMD16 ? PRED_ANY16H : PRED_ANY32H)));
      I2->setPredicate(I2_P);
      aBB->insertBefore(aInsertBeforePos, I2);
    }
  };

  auto createSIMD1Mov = [&](G4_BB *aBB, INST_LIST_ITER &aInsertBeforePos,
                            G4_RegVar *Dst, unsigned Dst_soff, G4_RegVar *Src,
                            unsigned Src_soff, G4_Type Ty) {
    G4_DstRegRegion *D = builder.createDst(Dst, 0, Dst_soff, 1, Ty);
    G4_SrcRegRegion *S = builder.createSrc(Src, 0, Src_soff, ScalarReg, Ty);
    G4_INST *tI =
        builder.createMov(g4::SIMD1, D, S, InstOpt_WriteEnable, false);
    aBB->insertBefore(aInsertBeforePos, tI);
    return tI;
  };

  auto initWAFlag = [&](G4_BB *aBB, INST_LIST_ITER &aInsertBeforePos,
                        G4_RegVar *aFlag, G4_Type aTy, bool &aFlagCreated,
                        bool &aFlagSaved, const bool aSaveFlag) {
    if (aFlagCreated) {
      // Reload the already-saved WAFlag
      vISA_ASSERT(aFlagSaved, "WAFlag should have been saved!");
      (void)createSIMD1Mov(aBB, aInsertBeforePos, aFlag, 0, SaveVar,
                           WAFlagSaveOff(aTy), aTy);
    } else {
      // Create a WAFlag for this BB
      createFlagFromCmp(aBB, aInsertBeforePos, aFlag, aTy);
      aFlagCreated = true;

      if (!aFlagSaved && aSaveFlag) {
        // save WAFlag
        (void)createSIMD1Mov(aBB, aInsertBeforePos, SaveVar, WAFlagSaveOff(aTy),
                             aFlag, 0, aTy);
        aFlagSaved = true;
      }
    }
  };

  // doPredicateInstWA()  : WA for a predicated inst without condMod
  //
  // flagVar : Var for WA flag for this BB:
  // currII:  iter to inst to which WA is applied.
  //   Given a predicated inst 'I'
  //        I :  (W&[+-]P) <inst> (8|M0) ...
  //      to:
  //        I0:  (W)           mov (1|M0) waTemp<0;1,0>    P
  //        I1:  (W&-flagVar)  mov (1|M0)  P  0 [+] | 0xffff [-]
  //        I :  (W&[+-]P)     <inst> (8|M0) ...                  [unchanged]
  //        I2:  (W&-flagVar)  mov (1|M0) P   waTemp<0;1,0>
  //
  // where the original predCtrl of P at 'I' shall remain unchanged.
  //
  auto doPredicateInstWA = [&](G4_BB *aBB, INST_LIST_ITER &aII,
                               G4_RegVar *aFlagVar) {
    G4_INST *I = *aII;
    G4_Predicate *P = I->getPredicate();
    vISA_ASSERT((P && !I->getCondMod()),
                "ICE: expect predicate and no flagModifier!");

    uint32_t flagBits =
        (P->getRightBound() - P->getLeftBound() + 1) + I->getMaskOffset();
    vISA_ASSERT(
        (16 * aFlagVar->getDeclare()->getRootDeclare()->getWordSize()) >=
            flagBits,
        "ICE[vISA]: WA's flagVar should not be smaller!");

    G4_Type Ty = (flagBits > 16) ? Type_UD : Type_UW;

    // I0:  (W) mov (1|M0) waTemp  P
    verifyRegVarSize(WATempVar, 4);
    (void)createSIMD1Mov(aBB, aII, WATempVar, 0, P->getTopDcl()->getRegVar(), 0,
                         Ty);

    // I1: (W&-flagVar)  mov (1|M0)  P  0 [+] | 0xffff [-]
    int64_t imm = (P->getState() == PredState_Plus ? 0 : 0xFFFFFFFF);
    G4_Imm *I1_s0 = builder.createImm(imm, Ty);
    G4_DstRegRegion *I1_d =
        builder.createDst(P->getTopDcl()->getRegVar(), 0, 0, 1, Ty);
    G4_Predicate *I1_flag = builder.createPredicate(PredState_Minus, aFlagVar,
                                                    0, getPredCtrl(UseAnyh));
    G4_INST *I1 =
        builder.createMov(g4::SIMD1, I1_d, I1_s0, InstOpt_WriteEnable, false);
    I1->setPredicate(I1_flag);
    aBB->insertBefore(aII, I1);

    // I : unchanged

    // I2: (W&-flagVar)  mov (1|M0) P   waTemp<0;1,0>
    auto nextII = std::next(aII);
    G4_INST *I2 = createSIMD1Mov(aBB, nextII, P->getTopDcl()->getRegVar(), 0,
                                 WATempVar, 0, Ty);
    G4_Predicate *I2_flag = builder.createPredicate(PredState_Minus, aFlagVar,
                                                    0, getPredCtrl(UseAnyh));
    I2->setPredicate(I2_flag);
  };

  // doFlagModifierSelInstWA : WA for sel/csel inst
  //   sel:  either predicate or condmod, not both
  //  csel:  no predicate, must have condMod
  // Both do not update flag.
  //
  // flagVar : WA flag for this BB
  // Before:
  //     I:  (W) sel.ge.f0.0  (1|M0)   r10.0<1>:f  r20.0<0;1,0>:f  0:f
  // After
  //     I:  (W) sel.ge.f0.0  (1|M0)  WATemp:f  r20.0<0;1,0>:f   0:f
  //     I0: (W&flagVar) mov  (1|M0)  r10.0<1>:f WATemp:f
  //
  auto doFlagModifierSelInstWA = [&](G4_BB *aBB, INST_LIST_ITER &aII,
                                     G4_RegVar *aFlagVar) {
    G4_INST *I = *aII;
    G4_DstRegRegion *dst = I->getDst();
    vISA_ASSERT(!isNull(dst), "ICE: expect dst to be non-null!");

    // Make sure that a temp, created in preRA, is big enough to hold data and
    // possible gap b/w data due to alignment/hw restriction.
    const uint16_t HS = dst->getHorzStride();
    uint32_t dst_bytes = I->getExecSize() * HS * dst->getTypeSize();
    verifyRegVarSize(WATempVar, dst_bytes);

    // I : (W) sel.ge.f0.0  (1|M0)  WATemp:f  r20.0<0;1,0>:f   0:f
    G4_DstRegRegion *I_d =
        builder.createDst(WATempVar, 0, 0, HS, dst->getType());
    I->setDest(I_d);

    // I0: (W&flagVar) mov  (1|M0)  r10.0<1>:f WATemp:f
    const RegionDesc *regionSave =
        builder.createRegionDesc(I->getExecSize(), HS, 1, 0);
    auto nextII = std::next(aII);
    G4_SrcRegRegion *I0_src0 =
        builder.createSrc(WATempVar, 0, 0, regionSave, dst->getType());
    G4_INST *I0 = builder.createMov(I->getExecSize(), dst, I0_src0,
                                    InstOpt_WriteEnable, false);
    G4_Predicate *I0_f = builder.createPredicate(PredState_Plus, aFlagVar, 0,
                                                 getPredCtrl(UseAnyh));
    I0->setPredicate(I0_f);
    aBB->insertBefore(nextII, I0);
  };

  // clang-format off
  // doFlagModifierInstWA : WA for an inst with flagModifier but no predicate.
  //
  // flagVar : WA flag for this BB.
  //    Before:
  //       I:  (W)  cmp (16|M16) (ne)P  D ....   // 32-bit flag
  //         or
  //           (W)  cmp (16|M0)  (ne)P  D ....   // 16-bit flag
  //
  //    After:
  //      (1) D = null (common)
  //           I0: (W)             mov (1|M0) WATemp   P
  //           I:  (W)             cmp (16|M16) (ne)P  ....
  //           I1: (W&-flagVar)    mov (1|M0)  P   WATemp
  //      (2) I's execMask is the same as flagVar's size
  //          (I's entire condMod is defined by I.)
  //           I0  (W)             mov (1|M0)  WATemp  P
  //           I1: (W)             mov (1|M0)  P   flagVar
  //            I: (W&P)           cmp (16|M0) (ne)P .....          // add predicate
  //           I2: (W&~flagVar)    mov (1|M0)  P  WATemp
  //      (3) otherwise(less common)
  //               Note that the sequence can only modify P that this cmp will
  //               change.
  //           I0: (W)             mov (1|M0)  WATemp  P
  //           I1: (W)             or  (1|M0)  P  P   <I's execMask>  // enable all
  //           I2: (W&~flagVar)    and (1|M0)  P  P   ~<I's execMask> // disable all
  //            I: (W&P)           cmp (16|M0) (ne)P .....            // add pred
  //           I3: (W&~flagVar)    mov (1|M0)  P  WATemp
  //
  // clang-format on
  auto doFlagModifierInstWA = [&](G4_BB *aBB, INST_LIST_ITER &aII,
                                  G4_RegVar *aFlagVar) {
    G4_INST *I = *aII;
    G4_CondMod *P = I->getCondMod();
    vISA_ASSERT((P && !I->getPredicate()),
                "ICE: expect flagModifier and no predicate!");

    // sel is specially handled in a different function.
    vASSERT(!(I->opcode() == G4_sel || I->opcode() == G4_csel));

    G4_Declare *modDcl = P->getTopDcl();
    G4_RegVar *modVar = modDcl->getRegVar();
    G4_Type Ty = (modDcl->getWordSize() > 1) ? Type_UD : Type_UW;
    G4_Type flagVarTy =
        (aFlagVar->getDeclare()->getWordSize() > 1 ? Type_UD : Type_UW);
    if (isNull(I->getDst())) { // case 1

      // I0: (W)        mov (1|M0) WATemp  P
      verifyRegVarSize(WATempVar, 4);
      (void)createSIMD1Mov(aBB, aII, WATempVar, 0, modVar, 0, Ty);

      // I : unchanged

      // I1: (W&-flagVar.anyh) mov (1|M0)  P  WATemp
      auto nextII = std::next(aII);
      G4_INST *I1 = createSIMD1Mov(aBB, nextII, modVar, 0, WATempVar, 0, Ty);
      G4_Predicate *I1_f = builder.createPredicate(PredState_Minus, aFlagVar, 0,
                                                   getPredCtrl(UseAnyh));
      I1->setPredicate(I1_f);

      return;
    }

    const uint32_t execMask = I->getExecLaneMask();
    vISA_ASSERT(
        (Ty == Type_UD || (execMask & 0xFFFF0000) == 0),
        "ICE: a flag used in an inst should not be smaller than the inst's "
        "execMask!");
    if (flagVarTy == Ty && ((execMask == 0xFFFF && Ty == Type_UW) ||
                            (execMask == 0xFFFFFFFF && Ty == Type_UD))) {
      // case 2 : entire mod is defined by 'I' !
      //
      // I0: (W)        mov (1|M0) WATemp  P
      verifyRegVarSize(WATempVar, 4);
      (void)createSIMD1Mov(aBB, aII, WATempVar, 0, modVar, 0, Ty);

      // I1: (W) mov (1|M0)  P  flagVar
      (void)createSIMD1Mov(aBB, aII, modVar, 0, aFlagVar, 0, Ty);

      // I: add the new predicate (must be the same as modDcl), for example:
      //    (W&P.anyh)       cmp (16|M0) (ne)P  ....
      G4_Predicate *I_P = builder.createPredicate(PredState_Plus, modVar, 0,
                                                  getPredCtrl(UseAnyh));
      I->setPredicate(I_P);

      // I2: (W&~flagVar.anyh)  mov (1|M0)  P  WATemp
      auto nextII = std::next(aII);
      G4_INST *I2 = createSIMD1Mov(aBB, nextII, modVar, 0, WATempVar, 0, Ty);
      G4_Predicate *I2_f = builder.createPredicate(PredState_Minus, aFlagVar, 0,
                                                   getPredCtrl(UseAnyh));
      I2->setPredicate(I2_f);

      return;
    }

    // case 3 (less common)
    //
    // I0: (W)        mov (1|M0) WATemp  P<0;1,0>
    verifyRegVarSize(WATempVar, 4);
    (void)createSIMD1Mov(aBB, aII, WATempVar, 0, modVar, 0, Ty);

    // I1: (W) or (1|M0)  P  P   ExecMask
    G4_SrcRegRegion *I1_s0 = builder.createSrc(modVar, 0, 0, ScalarReg, Ty);
    G4_Imm *I1_s1 = builder.createImm(execMask, Ty);
    G4_DstRegRegion *I1_d = builder.createDst(modVar, 0, 0, 1, Ty);
    G4_INST *I1 = builder.createBinOp(G4_or, g4::SIMD1, I1_d, I1_s0, I1_s1,
                                      InstOpt_WriteEnable, false);
    aBB->insertBefore(aII, I1);

    // I2: (W&~flagVar.anyh) and (1|M0)  P  P   ~ExecMask
    uint32_t negExecMask = (uint32_t)(~execMask);
    G4_SrcRegRegion *I2_s0 = builder.createSrc(modVar, 0, 0, ScalarReg, Ty);
    G4_Imm *I2_s1 = builder.createImm(negExecMask, Ty);
    G4_DstRegRegion *I2_d = builder.createDst(modVar, 0, 0, 1, Ty);
    G4_INST *I2 = builder.createBinOp(G4_and, g4::SIMD1, I2_d, I2_s0, I2_s1,
                                      InstOpt_WriteEnable, false);
    G4_Predicate *I2_f = builder.createPredicate(PredState_Minus, aFlagVar, 0,
                                                 getPredCtrl(UseAnyh));
    I2->setPredicate(I2_f);
    aBB->insertBefore(aII, I2);

    // I: add a new predicate, for example:
    //    (W&P)            cmp (16|M0)  (ne)P .....
    G4_Predicate *I_P =
        builder.createPredicate(PredState_Plus, modVar, 0, PRED_DEFAULT);
    I->setPredicate(I_P);

    // I3: (W&~flagVar.anyh)  mov (1|M0)  P  WATemp
    auto nextII = std::next(aII);
    G4_INST *I3 = createSIMD1Mov(aBB, nextII, modVar, 0, WATempVar, 0, Ty);
    G4_Predicate *I3_f = builder.createPredicate(PredState_Minus, aFlagVar, 0,
                                                 getPredCtrl(UseAnyh));
    I3->setPredicate(I3_f);
  };

  // clang-format off
  //  doPredicateAndFlagModifierInstWA : WA for inst with both predicate and
  //  condMod
  //
  //  flagVar : emask for this BB:
  //
  //    Before:
  //       I:  (W&[-]P)  and (16|M0) (ne)P  ....
  //
  //    After:
  //          I0:   (W)           mov (1|M0) WATemp  P
  //      Three cases
  //      case 1:  'I' defines entire P
  //          I1:   (W&-flagVar)  mov (1|M0) P  0 (for +p)| ExecMask (for -P) // disable all lanes
  //      case 2: +P
  //          I1    (W&-flagVar)  and (1|M0) P   P  ~execMask   // disable all lanes
  //      case 3: -P
  //          I1    (W&-flagVar)   or (1|M0) P   P  execMask    // disable all lanes
  //
  //       I:  (W&[-]P)         and (16|M0) (ne)P  ....    // unchanged
  //       I2: (W&-flagVar)     mov (1|M0)  P   WATemp
  //
  // clang-format on
  auto doPredicateAndFlagModifierInstWA = [&](G4_BB *aBB, INST_LIST_ITER &aII,
                                              G4_RegVar *aFlagVar) {
    G4_INST *I = *aII;
    [[maybe_unused]] G4_Predicate *P = I->getPredicate();
    [[maybe_unused]] G4_CondMod *M = I->getCondMod();
    vISA_ASSERT((P && M), "ICE: expect both predicate and flagModifier!");
    vISA_ASSERT(P->getTopDcl() == M->getTopDcl(),
                "ICE: both predicate and flagMod must be the same flag!");

    G4_Declare *modDcl = M->getTopDcl();
    G4_RegVar *modVar = modDcl->getRegVar();
    G4_Type Ty = (modDcl->getWordSize() > 1) ? Type_UD : Type_UW;

    // I0: (W)        mov (1|M0) WATemp  P
    verifyRegVarSize(WATempVar, 4);
    (void)createSIMD1Mov(aBB, aII, WATempVar, 0, modVar, 0, Ty);

    uint32_t execMask = I->getExecLaneMask();
    uint32_t negExecMask = (uint32_t)(~execMask);
    bool isPlusP = (P->getState() == PredState_Plus);
    G4_INST *I1 = nullptr;
    if ((Ty == Type_UD && execMask == 0xFFFFFFFF) ||
        (Ty == Type_UW && execMask == 0xFFFF)) {
      // case 1 : entire P are defined.
      // I1:  (W&-flagVar)  mov (1|M0) P  0 (for +p)| ExecMask (for -P)
      G4_DstRegRegion *I1_d = builder.createDst(modVar, 0, 0, 1, Ty);
      G4_Imm *I1_imm = builder.createImm(isPlusP ? 0 : execMask, Ty);
      I1 = builder.createMov(g4::SIMD1, I1_d, I1_imm, InstOpt_WriteEnable,
                             false);
      G4_Predicate *I1_f = builder.createPredicate(PredState_Minus, aFlagVar, 0,
                                                   getPredCtrl(UseAnyh));
      I1->setPredicate(I1_f);
      aBB->insertBefore(aII, I1);
    } else {
      // case 2 & 3
      //
      // case 2: +P
      //   I1:  (W&-flagVar)  and (1|M0) P   P  ~execMask
      // case 3: -P
      //   I1:  (W&-flagVar)   or (1|M0) P   P  execMask
      G4_DstRegRegion *I1_d = builder.createDst(modVar, 0, 0, 1, Ty);
      G4_SrcRegRegion *I1_s0 = builder.createSrc(modVar, 0, 0, ScalarReg, Ty);
      G4_Imm *I1_imm =
          builder.createImm((isPlusP ? negExecMask : execMask), Ty);
      G4_opcode opc1 = (isPlusP ? G4_and : G4_or);
      I1 = builder.createBinOp(opc1, g4::SIMD1, I1_d, I1_s0, I1_imm,
                               InstOpt_WriteEnable, false);
      G4_Predicate *I1_f = builder.createPredicate(PredState_Minus, aFlagVar, 0,
                                                   getPredCtrl(UseAnyh));
      I1->setPredicate(I1_f);
      aBB->insertBefore(aII, I1);
    }

    // No change to I

    // I2: (W&-flagVar)     mov (1|M0)  P   WATemp
    auto nextII = std::next(aII);
    G4_INST *I2 = createSIMD1Mov(aBB, nextII, modVar, 0, WATempVar, 0, Ty);
    G4_Predicate *I2_f = builder.createPredicate(PredState_Minus, aFlagVar, 0,
                                                 getPredCtrl(UseAnyh));
    I2->setPredicate(I2_f);
  };

  auto doSimpleInstWA = [&](G4_BB *aBB, INST_LIST_ITER &aII,
                            G4_RegVar *aFlagVar) {
    G4_INST *I = *aII;
    [[maybe_unused]] G4_Predicate *P = I->getPredicate();
    [[maybe_unused]] G4_CondMod *M = I->getCondMod();
    vISA_ASSERT((P == nullptr && M == nullptr),
                "ICE: expect neither pred nor condmod!");

    G4_Predicate *newPred = builder.createPredicate(PredState_Plus, aFlagVar, 0,
                                                    getPredCtrl(UseAnyh));
    I->setPredicate(newPred);
  };

  auto applyWAToInst = [&](G4_BB *aBB, INST_LIST_ITER &aII,
                           G4_RegVar *aFlagVar) {
    G4_INST *I = *aII;
    G4_Predicate *P = I->getPredicate();
    G4_CondMod *M = I->getCondMod();

    if ((I->opcode() == G4_sel || I->opcode() == G4_csel)) {
      // Not expecting null dst, as it is no-op
      if (!isNull(I->getDst())) {
        doFlagModifierSelInstWA(aBB, aII, aFlagVar);
      }
    } else if (P == nullptr && M == nullptr) {
      doSimpleInstWA(aBB, aII, aFlagVar);
    } else if (P != nullptr && M == nullptr) {
      doPredicateInstWA(aBB, aII, aFlagVar);
    } else if (P == nullptr && M != nullptr) {
      doFlagModifierInstWA(aBB, aII, aFlagVar);
    } else {
      doPredicateAndFlagModifierInstWA(aBB, aII, aFlagVar);
    }
  };

  for (G4_BB *BB : kernel.fg) {
    if ((BB->getBBType() & G4_BB_NM_WA_TYPE) == 0) {
      continue;
    }

    std::vector<INST_LIST_ITER> waInsts;
    // Set default for WAFlag's type, and it may be changed later.
    G4_Type WATy = (Simdsize == g4::SIMD32 ? Type_UD : Type_UW);
    // use anyh is preferred as it uses one instruction less.
    UseAnyh = true;

    // Collect all insts that need to apply WA. It also does:
    //    1. Determine WAFlag is UD or UW (simdsize isn't enough); and
    //    2. Check if WAFlag can use anyh or WAFlag must be all one's.
    for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II) {
      G4_INST *I = *II;
      if (isCandidate(I)) {
        waInsts.push_back(II);

        if ((I->getExecSize() + I->getMaskOffset()) > 16) {
          WATy = Type_UD;
        }
        if (UseAnyh &&
            (I->getExecSize() > Simdsize || I->getMaskOffset() != 0)) {
          UseAnyh = false;
        }
      }
    }

    if (waInsts.empty()) {
      continue;
    }

    FlagDefUse FlagDUInfo(BB);

    bool WAFlagCreated = false;
    bool WAFlagSaved = false;
    int ix = 0;
    const int NumWAInsts = (int)waInsts.size();
    while (ix < NumWAInsts) {
      INST_LIST_ITER currII = waInsts[ix];
      uint32_t WAFreg = 0xff;  // init to invalid number
      uint32_t WAFsreg = 0xff; // init to invalid number

      int nextIx;
      bool hasFreeFlag = FlagDUInfo.getBestFlagIfAvailable(
          waInsts, ix, nextIx, WATy, WAFreg, WAFsreg);
      if (hasFreeFlag) { // found available flag in [ix, nextIx).
        vASSERT(nextIx > ix);
        // Given
        //     (W) add (16|M0)  r10  r20  r30
        // Changed to
        //     1) (W) mov (1|M0) saveVar  f1.0
        //     2) <init waflag f1.0>
        //     3) apply WA to all inst in [ix, nextIx). "(W) add (16|M0)  r10
        //     r20  r30" is at ix 4) (W) mov (1|M0)  f1.0  saveVar
        G4_RegVar *WAFlagVar = getFlagDcl(WAFreg, WAFsreg, WATy)->getRegVar();
        WAFlagVar->setPhyReg(builder.phyregpool.getFlagAreg(WAFreg), WAFsreg);

        // 1) save the original flag for WAFlag.
        (void)createSIMD1Mov(BB, currII, SaveVar, 0, WAFlagVar, 0, WATy);

        // 2) init or reload WAFlag
        bool saveWAFlag = (nextIx < NumWAInsts);
        initWAFlag(BB, currII, WAFlagVar, WATy, WAFlagCreated, WAFlagSaved,
                   saveWAFlag);

        // 3) apply WA
        INST_LIST_ITER lastII = waInsts[nextIx - 1];
        INST_LIST_ITER nextII = std::next(lastII);
        for (int j = ix; j < nextIx; ++j) {
          currII = waInsts[j];
          applyWAToInst(BB, currII, WAFlagVar);
        }

        // 4) restore the saved original flag before the next inst.
        (void)createSIMD1Mov(BB, nextII, WAFlagVar, 0, SaveVar, 0, WATy);

        // set ix for the next wa inst.
        ix = nextIx;
      } else {
        uint32_t fr, fsr;
        G4_Type ty;

        // waInsts[ix] uses all flags. Need to save one to the reserved tmp.
        //   It is possible to have flag in src0, dst, and condMod/predicate.
        //   First, need to pick up one that is not used by condMod/predicate
        //   so that WAFlag can still work.
        G4_INST *I = *currII;
        G4_Predicate *P = I->getPredicate();
        G4_CondMod *M = I->getCondMod();
        G4_Operand *O_f = (P != nullptr ? (G4_Operand *)P : (G4_Operand *)M);
        G4_Operand *src0 = I->getSrc(0);
        G4_SrcRegRegion *sreg =
            ((!isNull(src0) && src0->isSrcRegRegion()) ? src0->asSrcRegRegion()
                                                       : nullptr);
        G4_DstRegRegion *dreg = I->getDst();
        if (O_f != nullptr) {
          [[maybe_unused]] bool isValid =
              FlagDefUse::getFlagRegAndSubreg(O_f, WAFreg, WAFsreg, ty);
          vISA_ASSERT(isValid,
                      "Flag should've been assigned physical reg already!");

          // WAFlag must use the other flag
          WAFreg = (WAFreg == 0 ? 1 : 0);
        } else {
          G4_Operand *O =
              (!isNull(sreg) && src0->isFlag())
                  ? (G4_Operand *)sreg
                  : (G4_Operand *)((!isNull(dreg) && dreg->isFlag()) ? dreg
                                                                     : nullptr);
          vISA_ASSERT(
              O != nullptr,
              "ICE: inst must have flag operands if it uses all flags!");

          [[maybe_unused]] bool isValid =
              FlagDefUse::getFlagRegAndSubreg(O, WAFreg, WAFsreg, ty);
          vISA_ASSERT(isValid,
                      "Flag should've been assigned physical reg already!");
        }

        // Save the entire flag, even though only the half is used.
        G4_RegVar *tVar = getFlagDcl(WAFreg, 0, Type_UD)->getRegVar();
        tVar->setPhyReg(builder.phyregpool.getFlagAreg(WAFreg), 0);

        // WAFlag. It can be UW (no tVar:UD). Uses 0 as sreg always in this
        // case.
        WAFsreg = 0;
        G4_RegVar *WAFlagVar = getFlagDcl(WAFreg, WAFsreg, WATy)->getRegVar();
        WAFlagVar->setPhyReg(builder.phyregpool.getFlagAreg(WAFreg), WAFsreg);

        // clang-format off
        // Assume that simdsize = 32 and currII is
        //    (W&f0.1)  or (1|M0) f1.0:uw  f1.1 0x101:uw
        // WA codes are:
        //    1) (W) mov (1|M0)  saveVar:ud   f1.0:ud
        //    2) <init waflag f1.0>
        //    3) (W&f0.1)   or (1|M0) saveVar:uw  saveVar.1:uw  0x101:uw  [WA will be applied]
        //    4) (W) mov (1|M0)  f1.0:ud  saveVar:ud                      [needed for dst change]
        // clang-format on

        // 1) save the original flag for WAFlag.
        (void)createSIMD1Mov(BB, currII, SaveVar, 0, tVar, 0, Type_UD);

        // 2) create WAFlag if not yet, or reload the WAFlag
        bool saveWAFlag = (ix != (NumWAInsts - 1));
        initWAFlag(BB, currII, WAFlagVar, WATy, WAFlagCreated, WAFlagSaved,
                   saveWAFlag);

        // 3) (1) Modify I; (2) apply WA
        INST_LIST_ITER nextII = std::next(currII);
        for (int i = 0; i < 2; ++i) {
          G4_Operand *O = (i == 0 ? (G4_Operand *)dreg : (G4_Operand *)sreg);
          if (!isNull(O) && O->isFlag()) {
            [[maybe_unused]] bool isValid = FlagDefUse::getFlagRegAndSubreg(O, fr, fsr, ty);
            vISA_ASSERT(isValid,
                        "Flag should've been assigned physical reg already!");

            if (fr == WAFreg) {
              // flag : either 2bytes at roff 0 or 1; or 4 bytes at roff 0
              vASSERT(fsr == 0 || O->getTypeSize() == 2);
              if (i == 0) {
                // dst
                G4_DstRegRegion *newDreg = builder.createDst(
                    SaveVar, 0, fsr, dreg->getHorzStride(), dreg->getType());
                I->setDest(newDreg);
              } else {
                // src0
                G4_SrcRegRegion *newSreg = builder.createSrc(
                    SaveVar, 0, fsr, sreg->getRegion(), sreg->getType());
                if (O->asSrcRegRegion() &&
                    O->asSrcRegRegion()->getModifier() != Mod_src_undef) {
                  newSreg->setModifier(O->asSrcRegRegion()->getModifier());
                }
                I->setSrc(newSreg, 0);
              }
            }
          }
        }
        applyWAToInst(BB, currII, WAFlagVar);

        // 4) Restore the original flag before the next inst
        (void)createSIMD1Mov(BB, nextII, tVar, 0, SaveVar, 0, Type_UD);

        // set ix for the next wa inst
        ++ix;
      }
    }
  }
  kernel.deleteEUFusionNoMaskWAInfo();
}

// Summary:
//   vISA assumes the call's target would be uniform within a thread. This is
//   consistent with hardware call instructions. Under EU fusion, a pair of
//   fused thread 0 and 1 might diverge, meaning that an indirect call invokes A
//   in thread 0 and invokes B in thread 1, which isn't supported by fused EU
//   hardware.
//
// This function is used to make sure each fused call will have a single target.
// As there are HW bugs in fused calls, this function will WA HW bugs as well.
// The general idea is:
//     Given:
//          (p) call  r5
//     Changed it to:
//          if (BigEU)
//             (p) call  r5
//          else   // SmallEU
//             (p) call  r5
//
// As HW has a bug in which call always runs (even no active channels) and it
// always uses BigEU's target as targets for both EUs. This causes several
// issues and the software WA is used to fix this harware bug. There are several
// cases:
//   1. For platforms that has NO HW fix (fusedCallWA 1), applying software WA
//   as described
//      below in "Details of 1",
//
//   2. For platforms that has the PARTIAL HW fix (fusedCallWA 2)
//      Any predicated call must be changed to unpredicated like the following:
//           (p) call ...
//       changed to
//           if (p)
//              call ...
//
//       This is done in Flowgraph::convertPredCall(), right after control-flow
//       is constructed.
//
//      2.1 for direct call like the following
//                (p) call r5
//
//           if (p)
//               if (BigEU)  // BigEU
//                  call  r5
//               else  // SmallEU
//                  call  r5
//   3. For platforms that have a full fix (if any) (fusedCallWA 0),
//      just do the following for indirect call.
//         (p) call r5
//         if (BigEU)  // BigEU
//             (p) call  r5
//         else  // SmallEU
//             (p) call  r5
//
//  This function handles 1) and duplicating call for BigEU and SmallEU.
//
// Details of 1
// ============
//  Under EU fusion,  assume that an indirect call invokes A in thread 0 and
//  invokes B in thread 1. Assume that these two threads are fused and run on a
//  pair of fused EUs {bigEU, smallEU}. The hardware will always invoke A: the
//  callee from thread 0 in bigEU even in else branch (in general case), which
//  is incorrect. To workaround this bug, we have to rely on the fact that cr0.2
//  is shared among the pair of fused EUs and copy thread 1's callee B into
//  thread 0 via cr0.2. In doing so, thread 1's callee can be invoked. The
//  details are as follows:
//
// clang-format off
//  before:
//  -------
//  BB:
//   pseudo_fcall (16)     V44(0,0)<0;1,0>:ud
//  nextBB:
//
//  Let Target = V44
//
//  after WA                                                    // Var Names
//  --------
//  BB:
//   (W)    mov (1 |M0) tmp<1>:ud  sr0.0<0;1,0>:ud              // I0
//   (W)    and (16|M0) (eq)F null<1>:uw tmp<0;1,0>:uw 0x80:uw  // I1
//   (W&~F) mov (1 |M0) cr0.2<1>:ud  Target<0;1,0>:ud           // I2
//   (W)    mov (1 |M0) smallEUTarget:ud  cr0.2<0;1,0>:ud       // I3
//   (W)    add (1 |M0) I4_IP:d   -ip:d  smallEUTarget:d        // I4_ip_start
//   (W)    add (1 |M0) I4Target:d   I4_IP:d  0x33333333:d      // I4_patch_add
//   (W)    add (1 |M0) I5_IP:d   -ip:d  Target:d               // I5_ip_start
//   (W)    add (1 |M0) I5Target:d   I5_IP:d  0x33333333:d      // I5_patch_add
//   (~F)   goto smallB0
//                             // [gotoSmallB0]
//  bigB0:
//          pseudo_fcall (16)     I5Target:ud                   // callI
//                                                                 (orig call)
//  bigB1:
//          goto nextBB                                         // gotoEnd
//  smallB0:
//          join nextBB                                         // joinSmall
//          pseudo_fcall (16)     I4Target<0;1,0>:ud // nCallI
//  smallB1:
//
//  nextBB:
//          join <nextJoin or null>                             // finalJoin
// clang-format on
//
// The BBs and those insts such as I4_patch_add/I5_patch_add, etc are added into
// m_indirectCallWAInfo so that finishFusedCallWA() can finish post-processing
// to patch the relative IP and others. If calla can be used,  no IP patching is
// needed. See code for details.
//
// In order to make the following to run always even through bigEU is off,
//    "(W)     mov (1 |M0)  smallEUTarget:ud   cr0.2<0;1,0>:ud"
// a special maskOff (M16) must be used to force NoMask to run no matter if the
// EU is off or on. This will be handled in finishFusedCallWA(). (See details in
// finishFusedCallWA(). To make it work, any kernel with indirect call is
// required to be simd16 or simd8, not simd32, so that M16 can be used to force
// running the inst always.)
//
void Optimizer::applyFusedCallWA() {
  auto updateSubroutineTableIfNeeded = [&](G4_BB *aLeadBB, G4_BB *aB0,
                                           G4_BB *aB1, G4_BB *aS0, G4_BB *aS1,
                                           G4_BB *aEndB_or_null) {
    if (int numFuncs = (int)fg.sortedFuncTable.size()) {
      for (int i = 0; i < numFuncs; ++i) {
        FuncInfo *pFInfo = fg.sortedFuncTable[i];
        vASSERT(pFInfo);
        auto &tBBs = pFInfo->getBBList();
        auto tBI = std::find(tBBs.begin(), tBBs.end(), aLeadBB);
        if (tBI != tBBs.end()) {
          // This is FuncInfo for the current func (including kernel entry func)
          // Make sure new BBs are in the FuncInfo's BBList.
          std::list<G4_BB *> toBeInserted;
          toBeInserted.push_back(aB0);
          toBeInserted.push_back(aB1);
          toBeInserted.push_back(aS0);
          toBeInserted.push_back(aS1);
          if (aEndB_or_null) {
            toBeInserted.push_back(aEndB_or_null);
          }
          tBBs.insert(tBI, toBeInserted.begin(), toBeInserted.end());

          // inc call count as a call is duplicated
          pFInfo->incrementCallCount();
          break;
        }
      }
    }
  };

  unsigned int fusedEUCallWA = builder.getuint32Option(vISA_fusedCallWA);
  // Only process call wa (fusedCallWA = 1) or indirect call is non-uniform
  if (!((fusedEUCallWA == 1) ||
        !builder.getOption(vISA_fusedCallUniform))) {
    return;
  }

  for (BB_LIST_ITER BI = fg.begin(), BE = fg.end(); BI != BE;) {
    BB_LIST_ITER currBI = BI;
    ++BI;

    G4_BB *BB = (*currBI);
    if (!BB->isEndWithFCall()) {
      continue;
    }
    G4_InstCF *callI = BB->back()->asCFInst();
    if (!callI->isIndirectCall()) {
      // direct call, no wa needed
      continue;
    }

    if (fusedEUCallWA == 2) {
      auto callInfo = builder.getFcallInfo(callI);
      vISA_ASSERT(callInfo, "call info absent for ifcall");
      if (callInfo->isUniform())
        continue;
    }

    // Assume fcall always have a single/fall-thru succ
    if (BI == BE || BB->Succs.size() != 1 || BB->Succs.back() != (*BI)) {
      // Skip! (Could this happen ?)
      continue;
    }

    BB_LIST_ITER nextBI = BI;
    G4_BB *origNextBB = (*nextBI);
    G4_BB *nextBB = origNextBB;
    G4_BB *newNextBB = nullptr;
    if (G4_INST *leadInst = nextBB->getFirstInst()) {
      if (leadInst->opcode() == G4_while || leadInst->opcode() == G4_endif) {
        // Cannot insert join, otherwise, label for while/endif would be wrong
        // Here, create a new empty BB so that we can add join into it.
        newNextBB = fg.createNewBBWithLabel("CallWA_EndBB");
        nextBI = fg.insert(nextBI, newNextBB);

        // Adjust control-flow
        fg.removePredSuccEdges(BB, nextBB);

        fg.addPredSuccEdges(BB, newNextBB, true);
        fg.addPredSuccEdges(newNextBB, nextBB, false);
        nextBB = newNextBB;

        newNextBB->setDivergent(BB->isDivergent());
        if (builder.hasFusedEUNoMaskWA()) {
          newNextBB->setBBType(G4_BB_NM_WA_TYPE);
        }
      }
    }
    G4_ExecSize simdsz = fg.getKernel()->getSimdSize();
    G4_SrcRegRegion *Target = callI->getSrc(0)->asSrcRegRegion();

    // Create BBs, two for each then (BigEU) and else (SmallEU) branches.
    G4_BB *bigB0 = fg.createNewBBWithLabel("CallWA_BigB0");
    G4_BB *bigB1 = fg.createNewBBWithLabel("CallWA_BigB1");
    G4_BB *smallB0 = fg.createNewBBWithLabel("CallWA_SmallB0");
    G4_BB *smallB1 = fg.createNewBBWithLabel("CallWA_SmallB1");
    // Note that nextBI points to the nextBB!
    fg.insert(nextBI, bigB0);
    fg.insert(nextBI, bigB1);
    fg.insert(nextBI, smallB0);
    fg.insert(nextBI, smallB1); // this is an empty BB. Might be needed for
                                // stack restore, etc.

    G4_Label *endLabel = nextBB->front()->getLabel();
    G4_INST *joinSmallB0 = builder.createCFInst(
        nullptr, G4_join, simdsz, endLabel, nullptr, InstOpt_NoOpt, false);
    smallB0->push_back(joinSmallB0);
    // Let SWSB skip this join when building SIMD CF.
    joinSmallB0->asCFInst()->setSWSBSkip(true);

    G4_Label *smallB0Label = smallB0->front()->getLabel();
    G4_INST *gotoEnd = builder.createCFInst(
        nullptr, G4_goto, simdsz, smallB0Label, endLabel, InstOpt_NoOpt, false);
    bigB1->push_back(gotoEnd);

    // Need to insert a join in nextBB
    // This join will never jump, thus set its JIP to nullptr.
    G4_INST *tjoin = nextBB->getFirstInst();
    if (tjoin == nullptr || tjoin->opcode() != G4_join) {
      G4_INST *finalJoin = builder.createCFInst(
          nullptr, G4_join, simdsz, nullptr, nullptr, InstOpt_NoOpt, false);
      if (tjoin == nullptr) {
        nextBB->insertBefore(nextBB->end(), finalJoin);
      } else {
        auto iter = std::find(nextBB->begin(), nextBB->end(), tjoin);
        nextBB->insertBefore(iter, finalJoin);
      }
    }

    fg.removePredSuccEdges(BB, nextBB);

    fg.addPredSuccEdges(BB, bigB0, true);
    fg.addPredSuccEdges(BB, smallB0, false);
    fg.addPredSuccEdges(bigB0, bigB1);
    fg.addPredSuccEdges(bigB1, nextBB);
    fg.addPredSuccEdges(smallB0, smallB1);
    fg.addPredSuccEdges(smallB1, nextBB, true);

    // To make RA know that the real inst can flow from bigB1 to smallB0
    // an edge is added from bigB1 to smallB0
    fg.addPredSuccEdges(bigB1, smallB0);

    // divergence property update
    //   new BBs's divergence is the same as BB's
    bool isDivergent = BB->isDivergent();
    bigB0->setDivergent(isDivergent);
    bigB1->setDivergent(isDivergent);
    smallB0->setDivergent(isDivergent);
    smallB1->setDivergent(isDivergent);

    // I0:  mov tmp  sr0.0
    G4_VarBase *V_sr0 = builder.phyregpool.getSr0Reg();
    G4_SrcRegRegion *I0_Src0 =
        builder.createSrc(V_sr0, 0, 0, builder.getRegionScalar(), Type_UD);
    G4_Declare *tmp = builder.createTempVar(1, Type_UD, Any, "tmpSr0");
    G4_DstRegRegion *I0_Dst =
        builder.createDst(tmp->getRegVar(), 0, 0, 1, Type_UD);
    G4_INST *I0 = builder.createInternalInst(
        nullptr, G4_mov, nullptr, g4::NOSAT, g4::SIMD1, I0_Dst, I0_Src0,
        nullptr, InstOpt_WriteEnable);

    // I1:  and  (e)F   tmp  0x80
    G4_Declare *F =
        builder.createTempFlag(simdsz > g4::SIMD16 ? 2 : 1, "euid2");
    G4_CondMod *F_cm = builder.createCondMod(Mod_e, F->getRegVar(), 0);
    G4_SrcRegRegion *I1_Src0 = builder.createSrc(
        tmp->getRegVar(), 0, 0, builder.getRegionScalar(), Type_UW);
    G4_Imm *Bit7 = builder.createImm(0x80, Type_UW);
    G4_INST *I1 = builder.createInternalInst(
        nullptr, G4_and, F_cm, g4::NOSAT,
        simdsz > g4::SIMD16 ? g4::SIMD32 : g4::SIMD16,
        builder.createNullDst(Type_UW), I1_Src0, Bit7, InstOpt_WriteEnable);

    if (builder.getuint32Option(vISA_fusedCallWA) != 1) {
      vASSERT(!builder.getOption(vISA_fusedCallUniform));
      // Just need to duplicate the call so that one is called under BigEU,
      // and the other is under SmallEU.

      BB->pop_back(); // unlink the call inst from BB
      BB->push_back(I0);
      BB->push_back(I1);

      I0->addDefUse(I1, Opnd_src0);

      G4_Predicate *pred_m1 =
          builder.createPredicate(PredState_Minus, F->getRegVar(), 0);
      G4_INST *gotoSmallB0 =
          builder.createCFInst(pred_m1, G4_goto, simdsz, smallB0Label,
                               smallB0Label, InstOpt_NoOpt, false);
      BB->push_back(gotoSmallB0);
      I1->addDefUse(gotoSmallB0, Opnd_pred);

      G4_Predicate *nPred(callI->getPredicate());
      G4_SrcRegRegion *nSrc = builder.createSrc(
          Target->getBase(), 0, 0, builder.getRegionScalar(), Type_UD);
      G4_INST *nCallI = builder.createInternalInst(
          nPred, callI->opcode(), nullptr, g4::NOSAT, callI->getExecSize(),
          nullptr, nSrc, nullptr, callI->getOption());
      (void)bigB0->push_back(callI);
      (void)smallB0->push_back(nCallI);

      // Need to create fcall info
      auto orig_fcallinfo = builder.getFcallInfo(callI);
      if (orig_fcallinfo) {
        builder.addFcallInfo(nCallI, orig_fcallinfo->getArgSize(),
                             orig_fcallinfo->getRetSize(),
                             orig_fcallinfo->isUniform());
      }
      // Might need to update subroutine table
      updateSubroutineTableIfNeeded(origNextBB, bigB0, bigB1, smallB0, smallB1,
                                    newNextBB);

      if (!fg.globalOpndHT.isOpndGlobal(Target)) {
        callI->removeDefUse(Opnd_src0);
      }
      fg.globalOpndHT.addGlobalOpnd(Target);
      fg.globalOpndHT.addGlobalOpnd(nSrc);

      // done with this indirect call.
      continue;
    }

    //
    // main call WA under fusedCallWA = 1
    //

    // I2:  (!flag) mov cr0.2  callee
    G4_VarBase *V_cr0 = builder.phyregpool.getCr0Reg();
    G4_DstRegRegion *I2_Dst = builder.createDst(V_cr0, 0, 2, 1, Type_UD);
    G4_SrcRegRegion *I2_Src0 = builder.createSrc(
        Target->getBase(), 0, 0, builder.getRegionScalar(), Type_UD);
    G4_Predicate *pred_m =
        builder.createPredicate(PredState_Minus, F->getRegVar(), 0);
    G4_INST *I2 = builder.createMov(g4::SIMD1, I2_Dst, I2_Src0,
                                    InstOpt_WriteEnable, false);
    I2->setPredicate(pred_m);

    // I3:   mov smallEUTarget  cr0.2
    //     Note that both operands of call need to be GRF aligned due to bug.
    //     With calla, we need to create grf-aligned sTargetDecl. With call, the
    //     relative ip temp, created later as I5Target, will be grf-aligned,
    //     thus, sTargetDecl here does not need to be grf-aligned.
    G4_SubReg_Align calleeAlign =
        builder.supportCallaRegSrc() ? builder.getGRFAlign() : Any;
    G4_Declare *sTargetDecl =
        builder.createTempVar(1, Type_UD, calleeAlign, "smallEUTarget");
    G4_DstRegRegion *I3_Dst =
        builder.createDst(sTargetDecl->getRegVar(), 0, 0, 1, Type_UD);
    G4_SrcRegRegion *I3_Src0 =
        builder.createSrc(V_cr0, 0, 2, builder.getRegionScalar(), Type_UD);
    G4_INST *I3 = builder.createMov(g4::SIMD1, I3_Dst, I3_Src0,
                                    InstOpt_WriteEnable, false);

    // Insert WA instructions
    BB->pop_back(); // unlink the call inst from BB
    BB->push_back(I0);
    BB->push_back(I1);
    BB->push_back(I2);
    BB->push_back(I3);

    // update local dataflow
    I0->addDefUse(I1, Opnd_src0);
    I1->addDefUse(I2, Opnd_pred);

    G4_INST *nCallI;
    if (builder.supportCallaRegSrc()) {
      (void)bigB0->push_back(callI);

      G4_Predicate *nPred(callI->getPredicate());
      G4_SrcRegRegion *nSrc = builder.createSrc(
          sTargetDecl->getRegVar(), 0, 0, builder.getRegionScalar(), Type_UD);
      nCallI = builder.createInternalInst(
          nPred, callI->opcode(), nullptr, g4::NOSAT, callI->getExecSize(),
          nullptr, nSrc, nullptr, callI->getOption());
      smallB0->push_back(nCallI);

      if (!fg.globalOpndHT.isOpndGlobal(Target)) {
        callI->removeDefUse(Opnd_src0);
      }
      fg.globalOpndHT.addGlobalOpnd(Target);
      fg.globalOpndHT.addGlobalOpnd(nSrc);

      kernel.m_maskOffWAInsts.insert(std::make_pair(I3, BB));
      kernel.m_indirectCallWAInfo.emplace(
          BB, IndirectCallWAInfo(bigB0, smallB0, nullptr, nullptr, nullptr,
                                 nullptr, nullptr, callI, nCallI));
      // BB, bigB0, smallB0 should not be deleted and its instructions shall
      // stay inside. Set BB type to G4_BB_KEEP_TYPE so the other optim passes
      // will not delete them.
      BB->setBBType(G4_BB_KEEP_TYPE);
      bigB0->setBBType(G4_BB_KEEP_TYPE);
      smallB0->setBBType(G4_BB_KEEP_TYPE);
    } else {
      // relative target:  need to patch offset after SWSB in
      // finishFusedCallWA()

      //
      //    I4_ip_start:   add rSmallIP  (-ip)  smallTarget
      //    I4_patch_add:  add I4Target  rSmallIP   -0x33333333
      //    I5_ip_start:   add rBigIP  (-ip) + bigTarget
      //    I5_patch_add:  add I5Target  rBigIP   -0x33333333
      //       where 0x33333333 should be the IP difference between I4_ip_start
      //       and nCallI (to I4Target), I5_ip_start and callI (I5Target),
      //       respectively. and it is patched later.
      // If IP WA is needed, will add the following:
      //    ip_wa_mov:     mov  tIP    0x89ABCDEF                : placeholder.
      //    I4_ip_start:   add  rSmallIP  -tIP  smallTarget
      //    I4_patch_add:  add  I4Target  rSmallIP   -0x33333333 : patch needed
      //    I5_ip_start:   add  rBigIP  -tIP  smallTarget
      //    I5_patch_add:  add  I5Target  rBigIP   -0x33333333   : patch needed
      //  where ip_wa_mov will be removed in finishFusedCallWA() with ip wa
      //  using in-place call.
      //
      G4_VarBase *V_ip = nullptr;
      G4_INST *ip_wa_placeholder = nullptr;
      if (builder.needIPWA()) {
        // Need 2 DWs (grf-aligned) as using IP WA needs 2 DWs (return IP and
        // call mask)
        G4_Declare *tIP_dcl =
            builder.createTempVar(2, Type_D, builder.getGRFAlign(), "tIP");
        V_ip = (G4_VarBase *)tIP_dcl->getRegVar();

        // placeholder mov makes sure tIP has a valid live range.
        G4_DstRegRegion *IP_WA_Dst = builder.createDst(V_ip, 0, 0, 1, Type_D);
        G4_Imm *IP_WA_Src0 = builder.createImm(0x89ABCDEF, Type_D);
        ip_wa_placeholder = builder.createMov(g4::SIMD1, IP_WA_Dst, IP_WA_Src0,
                                              InstOpt_WriteEnable, false);
        BB->push_back(ip_wa_placeholder);
      } else {
        V_ip = (G4_VarBase *)builder.phyregpool.getIpReg();
      }

      // SmallEU
      G4_Declare *I4_IP = builder.createTempVar(1, Type_D, Any, "rSmallIP");
      G4_DstRegRegion *I4_Dst =
          builder.createDst(I4_IP->getRegVar(), 0, 0, 1, Type_D);
      G4_SrcRegRegion *I4_Src0 = builder.createSrcRegRegion(
          Mod_Minus, Direct, V_ip, 0, 0, builder.getRegionScalar(), Type_D);
      G4_SrcRegRegion *I4_Src1 = builder.createSrc(
          sTargetDecl->getRegVar(), 0, 0, builder.getRegionScalar(), Type_D);
      G4_INST *I4_ip_start =
          builder.createBinOp(G4_add, g4::SIMD1, I4_Dst, I4_Src0, I4_Src1,
                              InstOpt_WriteEnable, false);

      G4_Declare *I4Target = builder.createTempVar(
          1, Type_D, builder.getGRFAlign(), "rSmallEUTarget");
      G4_DstRegRegion *I4_pDst =
          builder.createDst(I4Target->getRegVar(), 0, 0, 1, Type_D);
      G4_SrcRegRegion *I4_pSrc0 = builder.createSrc(
          I4_IP->getRegVar(), 0, 0, builder.getRegionScalar(), Type_D);
      G4_Imm *I4_pSrc1 =
          builder.createImm(0x33333333, Type_D); // to be patched later
      G4_INST *I4_patch_add =
          builder.createBinOp(G4_add, g4::SIMD1, I4_pDst, I4_pSrc0, I4_pSrc1,
                              InstOpt_WriteEnable, false);

      // BigEU
      G4_Declare *I5_IP = builder.createTempVar(1, Type_D, Any, "rBigIP");
      G4_DstRegRegion *I5_Dst =
          builder.createDst(I5_IP->getRegVar(), 0, 0, 1, Type_D);
      G4_SrcRegRegion *I5_Src0 = builder.createSrcRegRegion(
          Mod_Minus, Direct, V_ip, 0, 0, builder.getRegionScalar(), Type_D);
      G4_SrcRegRegion *I5_Src1 = builder.createSrc(
          Target->getBase(), 0, 0, builder.getRegionScalar(), Type_D);
      G4_INST *I5_ip_start =
          builder.createBinOp(G4_add, g4::SIMD1, I5_Dst, I5_Src0, I5_Src1,
                              InstOpt_WriteEnable, false);

      G4_Declare *I5Target = builder.createTempVar(
          1, Type_D, builder.getGRFAlign(), "rBigEUTarget");
      G4_DstRegRegion *I5_pDst =
          builder.createDst(I5Target->getRegVar(), 0, 0, 1, Type_D);
      G4_SrcRegRegion *I5_pSrc0 = builder.createSrc(
          I5_IP->getRegVar(), 0, 0, builder.getRegionScalar(), Type_D);
      G4_Imm *I5_pSrc1 =
          builder.createImm(0x33333333, Type_D); // to be patched later
      G4_INST *I5_patch_add =
          builder.createBinOp(G4_add, g4::SIMD1, I5_pDst, I5_pSrc0, I5_pSrc1,
                              InstOpt_WriteEnable, false);

      BB->push_back(I4_ip_start);
      BB->push_back(I4_patch_add);
      BB->push_back(I5_ip_start);
      BB->push_back(I5_patch_add);

      callI->setSrc(builder.createSrc(I5Target->getRegVar(), 0, 0,
                                      builder.getRegionScalar(), Type_UD),
                    0);
      (void)bigB0->push_back(callI);

      G4_Predicate *nPred(callI->getPredicate());
      G4_SrcRegRegion *nSrc = builder.createSrc(
          I4Target->getRegVar(), 0, 0, builder.getRegionScalar(), Type_UD);
      nCallI = builder.createInternalInst(
          nPred, callI->opcode(), nullptr, g4::NOSAT, callI->getExecSize(),
          nullptr, nSrc, nullptr, callI->getOption());
      smallB0->push_back(nCallI);

      I3->addDefUse(I4_ip_start, Opnd_src1);
      I4_ip_start->addDefUse(I4_patch_add, Opnd_src0);
      I5_ip_start->addDefUse(I5_patch_add, Opnd_src0);
      fg.globalOpndHT.addGlobalOpnd(I4_pDst);
      fg.globalOpndHT.addGlobalOpnd(I5_pDst);
      if (!fg.globalOpndHT.isOpndGlobal(Target)) {
        callI->copyDef(I2, Opnd_src0, Opnd_src0);
        callI->transferDef(I5_ip_start, Opnd_src0, Opnd_src1);
      }

      // add indirect call wa info
      kernel.m_indirectCallWAInfo.emplace(
          BB, IndirectCallWAInfo(bigB0, smallB0, ip_wa_placeholder, I4_ip_start,
                                 I4_patch_add, I5_ip_start, I5_patch_add, callI,
                                 nCallI));

      kernel.m_maskOffWAInsts.insert(std::make_pair(I3, BB));
      kernel.m_maskOffWAInsts.insert(std::make_pair(I4_ip_start, BB));
      kernel.m_maskOffWAInsts.insert(std::make_pair(I4_patch_add, BB));
    }

    G4_Predicate *pred_m1 =
        builder.createPredicate(PredState_Minus, F->getRegVar(), 0);
    G4_INST *gotoSmallB0 =
        builder.createCFInst(pred_m1, G4_goto, simdsz, smallB0Label,
                             smallB0Label, InstOpt_NoOpt, false);
    BB->push_back(gotoSmallB0);
    I1->addDefUse(gotoSmallB0, Opnd_pred);

    // Need to create fcall info
    auto orig_fcallinfo = builder.getFcallInfo(callI);
    if (orig_fcallinfo) {
      builder.addFcallInfo(nCallI, orig_fcallinfo->getArgSize(),
                           orig_fcallinfo->getRetSize(),
                           orig_fcallinfo->isUniform());
    }
    // Might need to update subroutine table
    updateSubroutineTableIfNeeded(origNextBB, bigB0, bigB1, smallB0, smallB1,
                                  newNextBB);

    // nomask wa property
    //   if BB is marked with NM_WA_TYPE, set all new BBs with NM_WA_TYPE
    //   if BB is not marked with NM_WA_TYPE and is divergent, mark the
    //   smallB0/B1
    //       as NM_WA_TYPE
    if (builder.hasFusedEUNoMaskWA()) {
      if ((BB->getBBType() & G4_BB_NM_WA_TYPE) != 0) {
        bigB0->setBBType(G4_BB_NM_WA_TYPE);
        bigB1->setBBType(G4_BB_NM_WA_TYPE);
        smallB0->setBBType(G4_BB_NM_WA_TYPE);
        smallB1->setBBType(G4_BB_NM_WA_TYPE);
      } else if (isDivergent) {
        smallB0->setBBType(G4_BB_NM_WA_TYPE);
        smallB1->setBBType(G4_BB_NM_WA_TYPE);
      }
    }
  }
}

// Convert vISA MULH dst:d src0:d src1:d into
//    mul acc0.0<1>:d src0:d src1:w
//    mach dst:d src0:d src1:d
// convert vISA mul dst:d src0:d src1:d into
//    mul acc0.0<1>:d src0:d src1:w
//    macl dst:d src0:d src1:d
void Optimizer::expandMulPostSchedule() {
  if (!VISA_WA_CHECK(builder.getPWaTable(), Wa_14013677893)) {
    return;
  }

  for (auto bb : kernel.fg) {
    for (INST_LIST_ITER it = bb->begin(); it != bb->end(); it++) {
      G4_INST *inst = *it;
      if (inst->opcode() != G4_mul && inst->opcode() != G4_mulh) {
        continue;
      }

      G4_Operand *src0 = inst->getSrc(0);
      G4_Operand *src1 = inst->getSrc(1);
      G4_DstRegRegion *dst = inst->getDst();

      if (dst->isAccReg()) {
        continue;
      }

      if (!IS_DTYPE(src0->getType()) || !IS_DTYPE(src1->getType()) ||
          !IS_DTYPE(dst->getType())) {
        continue;
      }

      vISA_ASSERT(inst->getSaturate() == g4::NOSAT,
                  "NOSAT is expected in mul/mulh expanding");
      vISA_ASSERT(inst->getCondMod() == nullptr,
                  "DW multiply does not support conditional modifiers");
      vISA_ASSERT(!src0->isSrcRegRegion() ||
                      src0->asSrcRegRegion()->getModifier() == Mod_src_undef,
                  "no src0 modifier is expected in mul/mulh expanding");
      vISA_ASSERT(!src1->isSrcRegRegion() ||
                      src1->asSrcRegRegion()->getModifier() == Mod_src_undef,
                  "no src1 modifier is expected in mul/mulh expanding");

      uint32_t origOptions = inst->getOption();
      G4_Predicate *origPredicate = inst->getPredicate();
      auto execSize = inst->getExecSize();
      auto tmpType =
          (IS_UNSIGNED_INT(src0->getType()) && IS_UNSIGNED_INT(src1->getType()))
              ? Type_UD
              : Type_D;

      // 1, create a new mul inst
      G4_DstRegRegion *accDstOpnd =
          builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, tmpType);
      auto newMul = builder.createBinOp(
          G4_mul, execSize, accDstOpnd, builder.duplicateOperand(src0),
          builder.duplicateOperand(src1), origOptions, false);
      bb->insertBefore(it, newMul);
      inst->copyDefsTo(newMul, false);
      // change the src1 of MUL from :d to :w
      HWConformity hwConf(builder, kernel);
      hwConf.fixMulSrc1(std::prev(it), bb);

      // 2, create a mach/macl inst
      G4_INST *maclOrMachInst = nullptr;
      if (inst->opcode() == G4_mul) {
        // create a macl inst
        maclOrMachInst = builder.createMacl(
            execSize, dst, builder.duplicateOperand(src0),
            builder.duplicateOperand(src1), origOptions, tmpType);
      } else if (inst->opcode() == G4_mulh) {
        // create a mach inst
        maclOrMachInst = builder.createMach(
            execSize, dst, builder.duplicateOperand(src0),
            builder.duplicateOperand(src1), origOptions, tmpType);
      }
      maclOrMachInst->setPredicate(origPredicate);
      *it = maclOrMachInst;
      inst->removeAllDefs();
      newMul->addDefUse(maclOrMachInst, Opnd_implAccSrc);

      // 3, always add a dummy mov after mach/macl for HW read suppresion W/A
      auto dummyMovSrc = builder.createSrc(dst->getBase(), dst->getRegOff(), 0,
                                           builder.getRegionScalar(), Type_D);
      G4_INST *dummyMov =
          builder.createMov(g4::SIMD1, builder.createNullDst(Type_D),
                            dummyMovSrc, InstOpt_WriteEnable, false);
      bb->insertAfter(it, dummyMov);
    }
  }
}

// SOA layout of dst:(dst_hi32:d, dst_lo32:d)
// if src2 is not immediate value of zero, then expand MADW((dst_hi32, dst_lo32)
// = src0 * src1 + src2) to:
//     mul  (16) acc0.0<1>:d    src0<1;1,0>:d    src1<2;1,0>:uw
//     mach (16) dst_hi32<1>:d  src0<1;1,0>:d    src1<1;1,0>:d
//     addc (16) dst_lo32<1>:d  acc0.0<1;1,0>:d  src2<1;1,0>:d     // Low 32
//     bits add  (16) dst_hi32<1>:d  acc0.0<1;1,0>:d  dst_hi32<1;1,0>:d // High
//     32 bits
// otherwise, expand to:
//     mul  (16) acc0.0<1>:d    src0<1;1,0>:d    src1<2;1,0>:uw
//     mach (16) dst_hi32<1>:d  src0<1;1,0>:d    src1<1;1,0>:d // High 32 bits
//     mov  (16) dst_lo32<1>:d  acc0.0<1;1,0>:d                // Low 32 bits
void Optimizer::expandMadwPostSchedule() {
  if (!VISA_WA_CHECK(builder.getPWaTable(), Wa_14013677893)) {
    return;
  }

  for (auto bb : kernel.fg) {
    for (INST_LIST_ITER it = bb->begin(); it != bb->end(); it++) {
      G4_INST *inst = *it;
      if (inst->opcode() != G4_madw) {
        continue;
      }

      // Unset a AccWrCtrl first.
      inst->setOptionOff(InstOpt_AccWrCtrl);

      G4_Operand *src0 = inst->getSrc(0);
      G4_Operand *src1 = inst->getSrc(1);
      G4_Operand *src2 = inst->getSrc(2);
      G4_DstRegRegion *dst = inst->getDst();

      vISA_ASSERT(inst->getSaturate() == g4::NOSAT,
                  "NOSAT is expected in mul/mulh/madw expanding");
      vISA_ASSERT(inst->getCondMod() == nullptr,
                  "DW multiply does not support conditional modifiers");
      vISA_ASSERT(!src0->isSrcRegRegion() ||
                      src0->asSrcRegRegion()->getModifier() == Mod_src_undef,
                  "no src0 modifier is expected in mul/mulh/madw expanding");
      vISA_ASSERT(!src1->isSrcRegRegion() ||
                      src1->asSrcRegRegion()->getModifier() == Mod_src_undef,
                  "no src1 modifier is expected in mul/mulh/madw expanding");
      vISA_ASSERT(IS_DTYPE(src0->getType()) && IS_DTYPE(src1->getType()) &&
                      IS_DTYPE(src2->getType()),
                  "only DW-type sources are supported");

      uint32_t origOptions = inst->getOption();
      G4_Predicate *origPredicate = inst->getPredicate();
      auto execSize = inst->getExecSize();
      G4_Type tmpType =
          (IS_UNSIGNED_INT(src0->getType()) &&
           IS_UNSIGNED_INT(src1->getType()) && IS_UNSIGNED_INT(src2->getType()))
              ? Type_UD
              : Type_D;

      // 1, create a new mul inst
      G4_DstRegRegion *accDstOpnd =
          builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, tmpType);
      auto newMul = builder.createBinOp(
          G4_mul, execSize, accDstOpnd, builder.duplicateOperand(src0),
          builder.duplicateOperand(src1), origOptions, false);
      auto startIter = bb->insertBefore(it, newMul);
      inst->copyDefsTo(newMul, false);
      // change the src1 of MUL from :d to :w
      HWConformity hwConf(builder, kernel);
      hwConf.fixMulSrc1(startIter, bb);

      // 2, create a mach/macl inst
      int DstHiRegOffset = (int)std::ceil(
          (float)(execSize * TypeSize(tmpType)) / kernel.getGRFSize());
      G4_DstRegRegion *dstHi32 =
          builder.createDst(dst->getBase(), dst->getRegOff() + DstHiRegOffset,
                            dst->getSubRegOff(), 1, tmpType);
      G4_INST *machInst = builder.createMach(
          execSize, dstHi32, builder.duplicateOperand(src0),
          builder.duplicateOperand(src1), origOptions, tmpType);

      machInst->setPredicate(origPredicate);
      *it = machInst;
      inst->removeAllDefs();
      newMul->addDefUse(machInst, Opnd_implAccSrc);

      auto endIter = it;
      // always add a dummy mov after mach/macl for HW read suppresion W/A
      auto dummyMovSrc =
          builder.createSrc(dst->getBase(), dst->getRegOff() + DstHiRegOffset,
                            0, builder.getRegionScalar(), Type_D);
      G4_INST *dummyMov =
          builder.createMov(g4::SIMD1, builder.createNullDst(Type_D),
                            dummyMovSrc, InstOpt_WriteEnable, false);
      endIter = bb->insertAfter(endIter, dummyMov);

      // optimize: only do multiply if src2 is imme 0
      if (src2->isImm() && src2->asImm()->getImm() == 0) {
        // 3, create a mov inst
        auto dstLo32 = builder.createDst(dst->getBase(), dst->getRegOff(),
                                         dst->getSubRegOff(), 1, tmpType);
        auto accSrcOpndMov = builder.createSrc(
            builder.phyregpool.getAcc0Reg(), 0, 0,
            execSize == g4::SIMD1 ? builder.getRegionScalar()
                                  : builder.getRegionStride1(),
            tmpType);
        auto movInst = builder.createMov(execSize, dstLo32, accSrcOpndMov,
                                         origOptions, false);
        movInst->setPredicate(origPredicate);
        endIter = bb->insertAfter(endIter, movInst);
      } else {
        // 3, create a addc inst
        auto dstLo32 = builder.createDst(dst->getBase(), dst->getRegOff(),
                                         dst->getSubRegOff(), 1, tmpType);
        auto accSrcOpnd = builder.createSrc(
            builder.phyregpool.getAcc0Reg(), 0, 0,
            execSize == g4::SIMD1 ? builder.getRegionScalar()
                                  : builder.getRegionStride1(),
            tmpType);
        auto addcInst = builder.createBinOp(
            G4_addc, execSize, dstLo32, accSrcOpnd,
            builder.duplicateOperand(src2), origOptions, false);
        addcInst->setPredicate(origPredicate);
        endIter = bb->insertAfter(endIter, addcInst);

        // 4, create a add inst
        auto src1Add = builder.createSrc(
            dstHi32->getBase(), dstHi32->getRegOff(), dstHi32->getSubRegOff(),
            execSize == g4::SIMD1 ? builder.getRegionScalar()
                                  : builder.getRegionStride1(),
            tmpType);
        auto addInst = builder.createBinOp(
            G4_add, execSize, builder.duplicateOperand(dstHi32),
            builder.duplicateOperand(accSrcOpnd), src1Add, origOptions, false);
        addInst->setPredicate(origPredicate);
        endIter = bb->insertAfter(endIter, addInst);
      }

      // split inst if execSize is larger than native execSize
      if (execSize > builder.getNativeExecSize()) {
        hwConf.splitDWMULInst(startIter, endIter, bb);
        it = startIter;
      }
    }
  }
}
void Optimizer::fixReadSuppressioninFPU0() {
  auto isFloatPipe = [](G4_INST *inst) -> bool {
    // There seems to be 2 implementations used to determine whether an
    // instruction would go to float pipe:
    // G4_INST::isFloatPipeInstructionXe() and HWConformity::isFloatOr64().
    // Only check the types of dst and src0 now.
    if (G4_DstRegRegion *dst = inst->getDst())
      return IS_TYPE_FLOAT_ALL(dst->getType());

    if (const G4_Operand *src = inst->getSrc(0))
      return IS_TYPE_FLOAT_ALL(src->getType());

    return false;
  };
  auto isRawMov = [](G4_INST *inst) -> bool {
    if (!inst->isRawMov())
      return false;

    if (inst->hasACCOpnd())
      return false;

    G4_Type dstType = inst->getDst()->getType();
    return IS_TYPE_FLOAT_ALL(dstType) && dstType != Type_DF;
  };

  auto isRawSel = [](G4_INST *inst) -> bool {
    if (inst->opcode() != G4_sel)
      return false;

    if (const G4_CondMod *condMod = inst->getCondMod()) {
      if (condMod->getMod() != Mod_ge && condMod->getMod() != Mod_l)
        return false;
    }

    if (inst->getSaturate())
      return false;

    if (inst->getSrc(0)->isSrcRegRegion() &&
        inst->getSrc(0)->asSrcRegRegion()->hasModifier())
      return false;

    if (inst->getSrc(1)->isSrcRegRegion() &&
        inst->getSrc(1)->asSrcRegRegion()->hasModifier())
      return false;

    G4_Type dstType = inst->getDst()->getType();
    G4_Type src0Type = inst->getSrc(0)->getType();
    return ((src0Type == dstType && dstType == Type_F) ||
            (src0Type == Type_HF && dstType == Type_HF));
  };

  auto isSPPath = [&](G4_INST *inst) -> bool {
    return (isRawMov(inst) && inst->getSrc(0)->getType() == Type_HF) ||
           (isRawSel(inst) && inst->getSrc(0)->getType() == Type_HF) ||
           (inst->getSrc(0) && inst->getSrc(0)->getType() == Type_DF &&
            inst->getDst() && inst->getDst()->getType() == Type_F);
  };

  G4_INST *prev = nullptr;
  bool isPrevOnSPPath = false;
  for (auto bb : fg) {
    for (auto it = bb->begin(), ie = bb->end(); it != ie; ++it) {
      G4_INST *cur = *it;
      // Only check the instruction that goes to fp pipe.
      if (!isFloatPipe(cur))
        continue;

      bool isCurOnSPPath = isSPPath(cur);
      // insert a dummy csel to invalidate the read suppression buffer
      // when the current instruction would switch buses while having
      // same source register and data type.
      if (prev && isPrevOnSPPath ^ isCurOnSPPath) {
        G4_SrcRegRegion *srcToFix = nullptr;
        int maxNumSrc = std::max(prev->getNumSrc(), cur->getNumSrc());
        for (int i = 0; i < maxNumSrc; ++i) {
          if (!prev || !prev->getSrc(i) || !prev->getSrc(i)->isSrcRegRegion())
            continue;
          if (!cur->getSrc(i) || !cur->getSrc(i)->isSrcRegRegion())
            continue;
          G4_SrcRegRegion *prevSrc = prev->getSrc(i)->asSrcRegRegion();
          G4_SrcRegRegion *curSrc = cur->getSrc(i)->asSrcRegRegion();
          if (*curSrc == *prevSrc) {
            srcToFix = curSrc;
            break;
          }
        }
        if (srcToFix) {
          const RegionDesc *region = builder.createRegionDesc(4, 4, 1);
          G4_Declare *decl = builder.createHardwiredDeclare(4, Type_F, 1, 0);
          G4_SrcRegRegion *src0 = fg.builder->createSrcRegRegion(decl, region);
          G4_SrcRegRegion *src1 = fg.builder->createSrcRegRegion(decl, region);
          G4_SrcRegRegion *src2 = fg.builder->createSrcRegRegion(decl, region);
          G4_DstRegRegion *dst = fg.builder->createDstRegRegion(decl, 1);
          G4_INST *cselInst = builder.createInternalInst(
              nullptr, G4_csel, nullptr, g4::NOSAT, g4::SIMD4, dst, src0, src1,
              src2, InstOpt_WriteEnable);
          bb->insertBefore(it, cselInst);
        }
      }
      prev = cur;
      isPrevOnSPPath = isCurOnSPPath;
    }
  }
}

void Optimizer::prepareDPASFuseRSWA() {
  vISA_ASSERT(builder.hasDPAS() && builder.hasDPASFuseRSWA(),
              "Expected the function is called only when WA is specified in "
              "WATable or options");

  kernel.fg.resetLocalDataFlowData();
  kernel.fg.localDataFlowAnalysis();

  BitSet GRFwriteByALU(kernel.getNumRegTotal(), false);
  builder.src1FirstGRFOfLastDpas.resize(kernel.getNumRegTotal());
  builder.src1FirstGRFOfLastDpas.clear();

  std::list<G4_INST *> dpasList;

  for (auto BI : fg) {
    G4_BB *BB = BI;
    G4_INST *lastDpas = nullptr;
    for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II) {
      G4_INST *I = *II;

      if (!I->isSend()) {
        G4_Operand *dst = I->getDst();
        if (dst && !dst->isNullReg() && dst->isGreg()) {
          unsigned int LB = 0;
          unsigned int RB = 0;

          LB = (unsigned int)(dst->getLinearizedStart() /
                              builder.numEltPerGRF<Type_UB>());
          RB = (unsigned int)(dst->getLinearizedEnd() /
                              builder.numEltPerGRF<Type_UB>());
          GRFwriteByALU.set(LB, RB);
        }
      }

      if (I->isDpas()) {
        dpasList.push_back(I);
        lastDpas = I;
      }
    }
    if (lastDpas != nullptr) {
      G4_Operand *src1Opnd = lastDpas->asDpasInst()->getSrc(1);
      unsigned int LB = (unsigned int)(src1Opnd->getLinearizedStart() /
                                       builder.numEltPerGRF<Type_UB>());
      builder.src1FirstGRFOfLastDpas.set(LB, true);
    }
  }
  vISA_ASSERT(!builder.src1FirstGRFOfLastDpas.isAllset(),
              "Do not expect the first GRF of src1 in last dpas inst of every "
              "BB touches all GRFs");

  for (auto I : dpasList) {
    bool found_src1_def = false;
    bool sendDefineOnly = true;
    for (auto i = I->def_begin(), E = I->def_end(); i != E; ++i) {
      if (i->second == Opnd_src1) {
        found_src1_def = true;
        auto defInst = i->first;
        if (!defInst->isSend()) {
          sendDefineOnly = false;
          kernel.setNeedDPASWA(true);
          I->asDpasInst()->setMayNeedWA(true);
        }
      }
    }

    if (sendDefineOnly) {
      G4_Operand *src1Opnd = I->asDpasInst()->getSrc(1);
      unsigned int LB = (unsigned int)(src1Opnd->getLinearizedStart() /
                                       builder.numEltPerGRF<Type_UB>());
      unsigned int RB = (unsigned int)(src1Opnd->getLinearizedEnd() /
                                       builder.numEltPerGRF<Type_UB>());

      if (!GRFwriteByALU.isEmpty(LB, RB)) {
        kernel.setNeedDPASWA(true);
        I->asDpasInst()->setMayNeedWA(true);
      }
    }

    if (!found_src1_def) {
      kernel.setNeedDPASWA(true);
      I->asDpasInst()->setMayNeedWA(true);
    }
  }
}

// Expand Intrinsic::BarrierWA instruction
void Optimizer::applyBarrierWA(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  if (!inst->isBarrierWAIntrinsic())
    return;

  // The dst of Intrinsic::BarrierWA instruction has 1 DW for saving existing
  // flag so WA can use it in the loop
  auto dst = inst->getDst();

  G4_RegVar *WAFlagVar = builder.createTempFlag(2, "WAFlagUD")->getRegVar();
  WAFlagVar->setPhyReg(builder.phyregpool.getF0Reg(), 0);

  // save f0.0:ud to dst.0:ud, then f0.0 can be used in the loop
  //   (W) mov(1) dst.0:ud f0.0:ud
  G4_DstRegRegion *dstMovForSave = builder.createDst(
      dst->getBase(), dst->getRegOff(), dst->getSubRegOff(), 1, Type_UD);
  G4_SrcRegRegion *srcMovForSave =
      builder.createSrc(WAFlagVar, 0, 0, builder.getRegionScalar(), Type_UD);
  auto saveInst = builder.createMov(g4::SIMD1, dstMovForSave, srcMovForSave,
                                    InstOpt_WriteEnable, false);
  vASSERT(dstMovForSave->getLinearizedStart() >= dst->getLinearizedStart() &&
          dstMovForSave->getLinearizedEnd() <= dst->getLinearizedEnd());
  bb->insertBefore(it, saveInst);

  // create label
  G4_Label *label = builder.createLocalBlockLabel("barrier_WA_loop");
  auto labelInst = builder.createLabelInst(label, false);
  bb->insertBefore(it, labelInst);

  // (W) and(1) (eq)f0.0 null:ud n0.0:ud 0x1:ud
  G4_DstRegRegion *nullDst = builder.createNullDst(Type_UD);
  G4_SrcRegRegion *src0And = builder.createSrc(
      builder.phyregpool.getN0Reg(), 0, 0, builder.getRegionScalar(), Type_UD);
  G4_CondMod *condMod = builder.createCondMod(Mod_e, WAFlagVar, 0);
  auto andInst = builder.createInternalInst(
      nullptr, G4_and, condMod, g4::NOSAT, g4::SIMD1, nullDst, src0And,
      builder.createImm(0x1, Type_UD), InstOpt_WriteEnable);
  bb->insertBefore(it, andInst);

  // (W&f0.0) while(1) loop
  G4_Predicate *pred = builder.createPredicate(PredState_Plus, WAFlagVar, 0);
  auto whileInst = builder.createInternalCFInst(
      pred, G4_while, g4::SIMD1, label, label, InstOpt_WriteEnable);
  bb->insertBefore(it, whileInst);

  // restore f0.0:ud from dst.0:ud
  //   mov(1) f0.0:ud dst.0:ud
  G4_DstRegRegion *dstMovForRestore = builder.createDst(WAFlagVar, Type_UD);
  G4_SrcRegRegion *srcMovForRestore =
      builder.createSrc(dst->getBase(), dst->getRegOff(), dst->getSubRegOff(),
                        builder.getRegionScalar(), Type_UD);
  auto restoreInst =
      builder.createMov(g4::SIMD1, dstMovForRestore, srcMovForRestore,
                        InstOpt_WriteEnable, false);
  *it = restoreInst;
}

// Expand Intrinsic::NamedBarrierWA instruction
void Optimizer::applyNamedBarrierWA(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;

  if (!inst->isNamedBarrierWAIntrinsic())
    return;

  // The dst of Intrinsic::NamedBarrierWA instruction has 3 DWs:
  //     dst.0:ud is for legalizing the barrier id which could be :b datatype
  //     or immediate.
  //     dst.1:ud is for generating the mask.
  //     dst.2:ud is for saving existing flag so WA can use it in the loop
  // The src0 of Intrinsic::NamedBarrierWA instruction is the barrier id.

  auto dst = inst->getDst();
  auto src = inst->getSrc(0);

  G4_RegVar *WAFlagVar = builder.createTempFlag(2, "WAFlagUD")->getRegVar();
  WAFlagVar->setPhyReg(builder.phyregpool.getF0Reg(), 0);

  // save f0.0:ud to dst.2:ud, then f0.0 can be used in the loop
  //   (W) mov(1) dst.2:ud f0.0:ud
  G4_DstRegRegion *dstMovForSave = builder.createDst(
      dst->getBase(), dst->getRegOff(), dst->getSubRegOff() + 2, 1, Type_UD);
  G4_SrcRegRegion *srcMovForSave =
      builder.createSrc(WAFlagVar, 0, 0, builder.getRegionScalar(), Type_UD);
  auto saveInst = builder.createMov(g4::SIMD1, dstMovForSave, srcMovForSave,
                                    InstOpt_WriteEnable, false);
  vASSERT(dstMovForSave->getLinearizedStart() >= dst->getLinearizedStart() &&
          dstMovForSave->getLinearizedEnd() <= dst->getLinearizedEnd());
  bb->insertBefore(it, saveInst);

  // (W) mov dst.1<1>:ud 0x1:ud
  G4_DstRegRegion *dstMov = builder.createDst(
      dst->getBase(), dst->getRegOff(), dst->getSubRegOff() + 1, 1, Type_UD);
  auto movInst =
      builder.createMov(g4::SIMD1, dstMov, builder.createImm(0x1, Type_UD),
                        InstOpt_WriteEnable, false);
  vASSERT(dstMov->getLinearizedStart() >= dst->getLinearizedStart() &&
          dstMov->getLinearizedEnd() <= dst->getLinearizedEnd());
  bb->insertBefore(it, movInst);

  // (W) mov dst.0<1>:ud src(barrierId):ud
  G4_DstRegRegion *dstMov2 = builder.createDst(dst->getBase(), dst->getRegOff(),
                                               dst->getSubRegOff(), 1, Type_UD);
  auto movInst2 =
      builder.createMov(g4::SIMD1, dstMov2, src, InstOpt_WriteEnable, false);
  vASSERT(dstMov2->getLinearizedStart() >= dst->getLinearizedStart() &&
          dstMov2->getLinearizedEnd() <= dst->getLinearizedEnd());
  bb->insertBefore(it, movInst2);

  // (W) shl(1) dst.1:ud dst.1:ud dst.0:ud
  G4_SrcRegRegion *src0Shl = builder.createSrc(
      dst->getBase(), dst->getRegOff(), dst->getSubRegOff() + 1,
      builder.getRegionScalar(), Type_UD);
  G4_SrcRegRegion *src1Shl =
      builder.createSrc(dst->getBase(), dst->getRegOff(), dst->getSubRegOff(),
                        builder.getRegionScalar(), Type_UD);
  auto shlInst =
      builder.createBinOp(G4_shl, g4::SIMD1, builder.duplicateOperand(dstMov),
                          src0Shl, src1Shl, InstOpt_WriteEnable, false);
  bb->insertBefore(it, shlInst);

  // create label
  G4_Label *label = builder.createLocalBlockLabel("barrier_WA_loop");
  auto labelInst = builder.createLabelInst(label, false);
  bb->insertBefore(it, labelInst);

  // (W) and(1) (eq)f0.0 null:ud n0.0:ud dst1.1:ud
  G4_DstRegRegion *nullDst = builder.createNullDst(Type_UD);
  G4_SrcRegRegion *src0And = builder.createSrc(
      builder.phyregpool.getN0Reg(), 0, 0, builder.getRegionScalar(), Type_UD);
  G4_SrcRegRegion *src1And = builder.duplicateOperand(src0Shl);
  G4_CondMod *condMod = builder.createCondMod(Mod_e, WAFlagVar, 0);
  auto andInst = builder.createInternalInst(nullptr, G4_and, condMod, g4::NOSAT,
                                            g4::SIMD1, nullDst, src0And,
                                            src1And, InstOpt_WriteEnable);
  bb->insertBefore(it, andInst);

  // (W&f0.0) while(1) loop
  G4_Predicate *pred = builder.createPredicate(PredState_Plus, WAFlagVar, 0);
  auto whileInst = builder.createInternalCFInst(
      pred, G4_while, g4::SIMD1, label, label, InstOpt_WriteEnable);
  bb->insertBefore(it, whileInst);

  // restore f0.0:ud from dst.2:ud
  //   mov(1) f0.0:ud dst.2:ud
  G4_DstRegRegion *dstMovForRestore = builder.createDst(WAFlagVar, Type_UD);
  G4_SrcRegRegion *srcMovForRestore = builder.createSrc(
      dst->getBase(), dst->getRegOff(), dst->getSubRegOff() + 2,
      builder.getRegionScalar(), Type_UD);
  auto restoreInst =
      builder.createMov(g4::SIMD1, dstMovForRestore, srcMovForRestore,
                        InstOpt_WriteEnable, false);
  *it = restoreInst;
}

// Insert IEEEExceptionTrap before EOT.
void Optimizer::insertIEEEExceptionTrap() {
  if (!fg.builder->getOption(vISA_AddIEEEExceptionTrap))
    return;

  for (auto bb : fg) {
    for (auto it = bb->begin(), ie = bb->end(); it != ie; ++it) {
      G4_INST *inst = *it;
      if (!inst->isEOT())
        continue;
      // Reserve 2 UD: one for sr0.1, the other for flag
      G4_Declare *tmp =
          builder.createTempVar(2, Type_UD, Even_Word, "ExTrapTemp");
      G4_INST *trap = builder.createIntrinsicInst(
          nullptr, Intrinsic::IEEEExceptionTrap, g4::SIMD1,
          builder.createDst(tmp->getRegVar(), 0, 0, 1, Type_UD), nullptr,
          nullptr, nullptr, InstOpt_WriteEnable, false);
      bb->insertBefore(it, trap);
    }
  }
}

// Expand IEEEExceptionTrap intrinsic as an infinite loop to catch any IEEE
// exception. Note that the IEEE exception trap enable bit should be set
// separately in CR initialization.
// TODO: Check if we can expand the trap into other inst like sync.host or
// illegal instruction to support this debug feature.
void Optimizer::expandIEEEExceptionTrap(INST_LIST_ITER it, G4_BB *bb) {
  G4_INST *inst = *it;
  vASSERT(inst->isIEEEExceptionTrap());

  auto dst = inst->getDst();
  // Get IEEE exception bits of state register where bits 5:0 of sr0.1:ud are
  // for IEEE exception.
  //   (W) mov (1) dst.0:ud sr0.1<0;1,0>:ud
  G4_DstRegRegion *tmpSR0Dot1Dst = builder.createDst(
      dst->getBase(), dst->getRegOff(), dst->getSubRegOff(), 1, Type_UD);
  G4_SrcRegRegion *SR0Dot1 = builder.createSrc(
      builder.phyregpool.getSr0Reg(), 0, 1, builder.getRegionScalar(), Type_UD);
  auto saveInst = builder.createMov(g4::SIMD1, tmpSR0Dot1Dst, SR0Dot1,
                                    InstOpt_WriteEnable, false);
  vASSERT(tmpSR0Dot1Dst->getLinearizedStart() >= dst->getLinearizedStart() &&
          tmpSR0Dot1Dst->getLinearizedEnd() <= dst->getLinearizedEnd());
  bb->insertBefore(it, saveInst);

  // Save f0.0:ud to dst.1:ud, then f0.0 can be used in the loop
  //   (W) mov(1) dst.1:ud f0.0:ud
  G4_RegVar *flagVar = builder.createTempFlag(1, "ex_trap_flag")->getRegVar();
  flagVar->setPhyReg(builder.phyregpool.getF0Reg(), 0);
  G4_DstRegRegion *tmpFlagDst = builder.createDst(
      dst->getBase(), dst->getRegOff(), dst->getSubRegOff() + 1, 1, Type_UD);
  G4_SrcRegRegion *flagSrc =
      builder.createSrc(flagVar, 0, 0, builder.getRegionScalar(), Type_UD);
  auto saveFlag = builder.createMov(g4::SIMD1, tmpFlagDst, flagSrc,
                                    InstOpt_WriteEnable, false);
  vASSERT(tmpFlagDst->getLinearizedStart() >= dst->getLinearizedStart() &&
          tmpFlagDst->getLinearizedEnd() <= dst->getLinearizedEnd());
  bb->insertBefore(it, saveFlag);

  // Check if any IEEE exception bit is set and update flag register.
  //   (W) and (1)  (ne)f0.0  tmpSR0Dot1  tmpSR0Dot1  0x3f:uw
  G4_SrcRegRegion *tmpSR0Dot1Src =
      builder.createSrc(dst->getBase(), dst->getRegOff(), dst->getSubRegOff(),
                        builder.getRegionStride1(), Type_UD);
  auto andInst = builder.createInternalInst(
      nullptr, G4_and, builder.createCondMod(Mod_ne, flagVar, 0), g4::NOSAT,
      g4::SIMD1, builder.duplicateOperand(tmpSR0Dot1Dst), tmpSR0Dot1Src,
      builder.createImm(0x3f, Type_UW), InstOpt_WriteEnable);
  bb->insertBefore(it, andInst);

  // Create label
  G4_Label *label = builder.createLocalBlockLabel("ex_trap_loop");
  auto labelInst = builder.createLabelInst(label, false);
  bb->insertBefore(it, labelInst);

  // Create a trap as infinite loop if flag register is set.
  //   (W&f0.0) while (1)  ex_trap_loop
  auto whileInst = builder.createInternalCFInst(
      builder.createPredicate(PredState_Plus, flagVar, 0), G4_while, g4::SIMD1,
      label, label, InstOpt_WriteEnable);
  bb->insertBefore(it, whileInst);

  // Restore flag register.
  //   (W) mov(1) f0.0:ud dst.1:ud
  G4_DstRegRegion *flagDst = builder.createDst(flagVar, Type_UD);
  G4_SrcRegRegion *tmpFlagSrc = builder.createSrc(
      dst->getBase(), dst->getRegOff(), dst->getSubRegOff() + 1,
      builder.getRegionScalar(), Type_UD);
  auto restoreFlag = builder.createMov(g4::SIMD1, flagDst, tmpFlagSrc,
                                       InstOpt_WriteEnable, false);
  *it = restoreFlag;
}

// For a subroutine, insert a dummy move with {Switch} option immediately
// before the first non-label instruction in BB. Otherwie, for a following
// basic block, insert a dummy move before *any* instruction to ensure that
// no instruction should be placed between the targe jip/uip label and its
// associated instruction.
void Optimizer::addSwitchOptionToBB(G4_BB *bb, bool isSubroutine) {
  auto instIter = bb->begin();
  if (isSubroutine) {
    for (auto instEnd = bb->end(); instIter != instEnd; ++instIter) {
      G4_INST *bbInst = *instIter;
      if (!bbInst->isLabel()) {
        break;
      }
    }
  }

  if (instIter != bb->end() && ((*instIter)->getOption() & InstOpt_Switch)) {
    // this BB is already processed, skip
    return;
  }

  // mov (1) null<1>:ud r0.0<0;1,0>:ud {Switch}
  G4_DstRegRegion *movDst = builder.createNullDst(Type_UD);
  G4_SrcRegRegion *movSrc = builder.createSrcRegRegion(
      builder.getBuiltinR0(), builder.getRegionScalar());
  G4_INST *movInst =
      builder.createMov(g4::SIMD1, movDst, movSrc, InstOpt_WriteEnable, false);
  movInst->setOptionOn(InstOpt_Switch);
  bb->insertBefore(instIter, movInst);
}

void Optimizer::linePlaneWA(G4_INST *inst) {
  // Putting it here instead of in HW confomrity because we need original src0
  // region in scheduler to calculate RB correctly. Otherwise setup moves for
  // src0 get scheduled after instruction
  //
  // HW check #12: Check and correct the first operand for line instruction
  // Actually it must be a replicated stream of 4 contiguous elements.
  // That means <0;4,1> region. But in asm code it must be presented as
  // replicated scalar - <0;1,0>.
  if (inst->opcode() == G4_line || inst->opcode() == G4_pln) {
    G4_Operand *src = inst->getSrc(0);
    const RegionDesc *rd =
        src->isSrcRegRegion() ? src->asSrcRegRegion()->getRegion() : NULL;
    vISA_ASSERT(rd != NULL, " Src0 of line inst is not regregion. ");
    if (rd->isScalar()) {
      return;
    }
    vISA_ASSERT((rd->vertStride == 0 || rd->vertStride == 4) && rd->width == 4,
                "Unexpected region for the first line operand.");

    // create a new rd for src0
    const RegionDesc *new_rd = builder.getRegionScalar();
    src->asSrcRegRegion()->setRegion(builder, new_rd);
  }
}

//
// This inserts two dummy moves to clear flag dependencies before EOT:
// mov(1) null:ud f0.0<0;1,0>:ud{ Align1, Q1, NoMask }
// mov(1) null:ud f1.0<0;1,0>:ud{ Align1, Q1, NoMask }
// This is done if f0/f1 is ever defined in a BB but not used in it, as we
// conservatively assume that the flag may be undefined when the EOT is reached.
// Note that USC only does this if EOT is inside control flow, i.e., EOT is an
// early exit
//
void Optimizer::clearARFDependencies() {
  auto flagToInt = [](G4_Areg *areg) {
    vISA_ASSERT(areg->isFlag(), "expect F0 or F1");
    return areg->getArchRegType() == AREG_F0 ? 0 : 1;
  };
  // see if F0 and F1 are ever defined but not used in the same BB
  bool unusedFlag[2]; // f0 and f1
  unusedFlag[0] = unusedFlag[1] = false;
  for (auto bb : fg) {
    bool unusedFlagLocal[2]; // f0 and f1
    unusedFlagLocal[0] = unusedFlagLocal[1] = false;

    for (auto inst : *bb) {
      if (inst->isEOT()) {
        // EOT should be the last inst in BB.
        continue;
      }

      // check predicate source
      if (inst->getPredicate()) {
        G4_VarBase *flag = inst->getPredicate()->getBase();
        if (flag->isRegVar()) {
          G4_Areg *areg = flag->asRegVar()->getPhyReg()->asAreg();
          unusedFlagLocal[flagToInt(areg)] = false;
        }
      }

      // check explicit source
      for (int i = 0; i < inst->getNumSrc(); ++i) {
        if (inst->getSrc(i) && inst->getSrc(i)->isSrcRegRegion() &&
            inst->getSrc(i)->isFlag()) {
          G4_SrcRegRegion *src = inst->getSrc(i)->asSrcRegRegion();
          if (src->getBase()->isRegVar()) {
            G4_Areg *flag = src->getBase()->asRegVar()->getPhyReg()->asAreg();
            unusedFlagLocal[flagToInt(flag)] = false;
          }
        }
      }

      // check explicit dst
      if (inst->getDst() && inst->getDst()->isFlag()) {
        // flag is an explicit dst
        G4_DstRegRegion *dst = inst->getDst();
        if (dst->getBase()->isRegVar()) {
          G4_Areg *flag = dst->getBase()->asRegVar()->getPhyReg()->asAreg();
          unusedFlagLocal[flagToInt(flag)] = true;
        }
      }
      // check cond mod
      else if (G4_VarBase *flag = inst->getCondModBase()) {
        if (flag->isRegVar()) {
          G4_Areg *areg = flag->asRegVar()->getPhyReg()->asAreg();
          unusedFlagLocal[flagToInt(areg)] = true;
        }
      }
    }

    if (unusedFlagLocal[0] && unusedFlag[0] == false) {
      unusedFlag[0] = true;
    }

    if (unusedFlagLocal[1] && unusedFlag[1] == false) {
      unusedFlag[1] = true;
    }

    if (unusedFlag[0] && unusedFlag[1]) {
      break;
    }
  }

  if (unusedFlag[0] || unusedFlag[1]) {
    for (auto bb : fg) {
      if (bb->size() == 0) {
        return;
      }
      G4_INST *inst = bb->back();
      if (inst->isEOT()) {
        auto instIter = bb->end();
        --instIter;
        if (unusedFlag[0]) {
          G4_SrcRegRegion *flagSrc =
              builder.createSrc(builder.phyregpool.getF0Reg(), 0, 0,
                                builder.getRegionScalar(), Type_UD);
          G4_DstRegRegion *nullDst = builder.createNullDst(Type_UD);
          G4_INST *inst = builder.createMov(g4::SIMD1, nullDst, flagSrc,
                                            InstOpt_WriteEnable, false);
          bb->insertBefore(instIter, inst);
        }
        if (unusedFlag[1]) {
          G4_SrcRegRegion *flagSrc =
              builder.createSrc(builder.phyregpool.getF1Reg(), 0, 0,
                                builder.getRegionScalar(), Type_UD);
          G4_DstRegRegion *nullDst = builder.createNullDst(Type_UD);
          G4_INST *inst = builder.createMov(g4::SIMD1, nullDst, flagSrc,
                                            InstOpt_WriteEnable, false);
          bb->insertBefore(instIter, inst);
        }
      }
    }
  }
}
void Optimizer::mulMacRSWA() {
  auto hasGRFOverlap = [this](G4_Operand *A, G4_Operand *B) {
    if (A->isNullReg() || !A->isGreg())
      return false;
    if (B->isNullReg() || !B->isGreg())
      return false;

    unsigned LB1 =
        A->getLinearizedStart() / fg.builder->numEltPerGRF<Type_UB>();
    unsigned RB1 = A->getLinearizedEnd() / fg.builder->numEltPerGRF<Type_UB>();
    unsigned LB2 =
        B->getLinearizedStart() / fg.builder->numEltPerGRF<Type_UB>();
    unsigned RB2 = B->getLinearizedEnd() / fg.builder->numEltPerGRF<Type_UB>();

    return (RB2 >= LB1 && RB1 >= LB2);
  };

  auto isBothMulClass = [](G4_INST *inst1, G4_INST *inst2) {
    return (inst1->opcode() == G4_mul || inst1->opcode() == G4_mac) &&
           (inst2->opcode() == G4_mul || inst2->opcode() == G4_mac);
  };

  auto isBothMaclClass = [](G4_INST *inst1, G4_INST *inst2) {
    // In vISA, only G4_mach will be used. IGA will change it G4_macl according
    // to certain conditions.
    return (inst1->opcode() == G4_mach) &&
           (inst2->opcode() == G4_mach);
  };

  auto checkFlatRegRegionFunc =
      [](uint8_t dstStrideInBytes, uint8_t dstSubRegOffInBytes,
         uint8_t srcStrideInBytes, uint8_t srcSubRegOffInBytes,
         uint8_t exChannelWidth) -> bool {
    return ((dstSubRegOffInBytes == srcSubRegOffInBytes) &&
            (dstStrideInBytes == srcStrideInBytes) &&
            (dstStrideInBytes % exChannelWidth == 0));
  };

  G4_INST *prevInst = nullptr;
  for (auto bb : fg) {
    INST_LIST_ITER ii = bb->begin();

    while (ii != bb->end()) {
      G4_INST *inst = *ii;

      if (!inst->isIntegerPipeInstructionXe()) {
        ii++;
        continue;
      }

      if (!prevInst) {
        prevInst = inst;
        ii++;
        continue;
      }

      uint8_t exChannelWidth = (uint8_t)TypeSize(inst->getExecType());

      // Issue 1:
      // MUL opcode class = {MUL, MAC}
      // MACL opcode class = {MACL, MACH}
      //
      // Issue is present for MUL opcode class  OR MACL opcode class (both
      // prev/current instruction should belong to the same opcode class)
      // 1. prev instructions src1 has REGIONING/SCALAR
      // 2. current instruction src1 is FLAT and shares the same src1 as prev
      //
      // instruction Issue is not present for below cases.
      // 1. prev instruction is FLAT and current instruction has
      // REGIONING/SCALAR
      // 2. prev/current both are FLAT
      // 3. prev/current both has REGIONING/SCALAR
      // 4. One instruction is in MUL opcode class and the other instruction
      // is in MACL opcode class
      if (isBothMulClass(prevInst, inst) || isBothMaclClass(prevInst, inst)) {
        G4_Operand *prevSrc1 = prevInst->getSrc(1);
        G4_Operand *curSrc1 = inst->getSrc(1);

        if (prevSrc1 && prevSrc1->isGreg() && prevSrc1->isSrcRegRegion() &&
            curSrc1 && curSrc1->isGreg() &&
            curSrc1->isSrcRegRegion()) { // All regions

          if (!prevSrc1->asSrcRegRegion()->isFlatRegRegion(
                  exChannelWidth, checkFlatRegRegionFunc) &&
              curSrc1->asSrcRegRegion()->isFlatRegRegion(
                  exChannelWidth, checkFlatRegRegionFunc) &&
              hasGRFOverlap(
                  prevSrc1,
                  curSrc1)) { // none flat vs flat regions, and overlap
            // WorkAround: Insert dummy instruction that can break src1 RS
            // chain between regioning MUL instruction and FLAT MULK
            // instruction (IMMEDIATE operand can be used  for src1 to break
            // the RS chain)
            insertDummyAdd(bb, ii);
          }
        }
      }

      // Issue 2
      // prev.instruction is non-MUL opcode class instruction AND non-MACL
      // opcode class instruction has(FLAT or Regioning / Scalar) src1 and
      // current Instruction is MACL opcode class
      // instruction AND has FLAT regioning AND shares the same src1 has the
      // prev.instruction,
      if (inst->opcode() == G4_mach) {
        G4_Operand *prevSrc1 = prevInst->getSrc(1);
        G4_Operand *curSrc1 = inst->getSrc(1);

        if (prevSrc1 && prevSrc1->isGreg() && prevSrc1->isSrcRegRegion() &&
            curSrc1 && curSrc1->isGreg() && curSrc1->isSrcRegRegion()) {
          if (prevInst->opcode() != G4_mach && prevInst->opcode() != G4_mul &&
              prevInst->opcode() != G4_mac) {
            if (curSrc1->asSrcRegRegion()->isFlatRegRegion(
                    exChannelWidth, checkFlatRegRegionFunc) &&
                hasGRFOverlap(prevSrc1, curSrc1)) {
              insertDummyAdd(bb, ii, 1);
            }
          }
        }
      }

      prevInst = inst;
      ii++;
    }
  }
}

// change the send src0 region to be consistent with assembler expectation
// We do it here instead of HW conformity since they only affect binary encoding
// ToDo: this should not be necessary anymore, should see if we can remove
void Optimizer::fixSendSrcRegion(G4_INST *inst) {
  if (inst->isSend() && inst->getSrc(0) != NULL) {
    const RegionDesc *newDesc = NULL;
    uint8_t execSize = inst->getExecSize();
    if (execSize == 1) {
      newDesc = builder.getRegionScalar();
    } else if (execSize > 8) {
      newDesc = builder.getRegionStride1();
    } else {
      newDesc = builder.createRegionDesc(execSize, execSize, 1);
    }
    inst->getSrc(0)->asSrcRegRegion()->setRegion(builder, newDesc);
  }
}

// some workaround for HW restrictions.  We apply them here so as not to affect
// optimizations, RA, and scheduling
void Optimizer::HWWorkaround() {
  // Ensure the first instruction of a stack function has switch option.
  if (fg.getIsStackCallFunc() &&
      VISA_WA_CHECK(builder.getPWaTable(), WaThreadSwitchAfterCall)) {
    addSwitchOptionToBB(fg.getEntryBB(), true);
  }

  DPASSrc2RSCache src2GRFCache;
  // set physical pred/succ as it's needed for the call WA
  fg.setPhysicalPredSucc();
  const bool scheduleFenceCommit =
      builder.getOption(vISA_scheduleFenceCommit) &&
      builder.getPlatform() >= GENX_TGLLP;
  BB_LIST_ITER ib, bend(fg.end());
  for (ib = fg.begin(); ib != bend; ++ib) {
    G4_BB *bb = (*ib);
    INST_LIST_ITER ii = bb->begin();

    while (ii != bb->end()) {
      G4_INST *inst = *ii;

      G4_InstSend *sendInst = inst->asSendInst();
      if (sendInst && sendInst->isFence() &&
          !builder.getOption(vISA_skipFenceCommit)) {
        addFenceCommit(ii, bb, scheduleFenceCommit);
      }

      // To solve truncation issue in compaction table implementation
      if (VISA_WA_CHECK(builder.getPWaTable(), Wa_22010811838) &&
          inst->isDpas()) {
        G4_InstDpas *dpasInst = inst->asDpasInst();
        GenPrecision p = dpasInst->getSrc1Precision();
        if (p == GenPrecision::S8 || p == GenPrecision::S4 ||
            p == GenPrecision::S2 || p == GenPrecision::BF16) {
          dpasInst->setOptionOn(InstOpt_NoCompact);
        }
      }
      if (inst->isCall() || inst->isFCall()) {
        if (VISA_WA_CHECK(builder.getPWaTable(), WaThreadSwitchAfterCall)) {
          // WA:
          // A call instruction must be followed by an instruction that supports
          // Switch. When call takes a jump, the first instruction must have a
          // Switch.
          BB_LIST_ITER nextBBIter = ib;
          ++nextBBIter;
          if (nextBBIter != bend) {
            addSwitchOptionToBB(*nextBBIter, false);
          }
          // also do this for call target
          addSwitchOptionToBB(bb->Succs.front(), true);
        }
      }

      // we must set {Switch} if the instruction updates ARF with no scoreboard
      {
        G4_DstRegRegion *dst = inst->getDst();
        if (dst != nullptr && dst->getBase()->noScoreBoard()) {
          inst->setOptionOn(InstOpt_Switch);
        }
      }

      if (inst->isSend() && !inst->isNoPreemptInst() &&
          builder.needsNoPreemptR2ForSend()) {
        G4_Operand *Src0 = inst->getSrc(0);
        if (Src0 && Src0->isGreg()) {
          unsigned LB = Src0->getLinearizedStart();
          if (LB == 2 * kernel.numEltPerGRF<Type_UB>()) {
            inst->setOptionOn(InstOpt_NoPreempt);
          }
        }
      }

      if (builder.hasFdivPowWA() && inst->isMath() &&
          (inst->asMathInst()->getMathCtrl() == MATH_FDIV ||
           inst->asMathInst()->getMathCtrl() == MATH_POW)) {
        INST_LIST_ITER nextIter = ii;
        nextIter++;
        if (nextIter == bb->end()) {
          break;
        }
        // check next inst
        G4_INST *nextInst = *nextIter;
        if (!nextInst->isSend() && nextInst->getDst() &&
            !nextInst->hasNULLDst() && nextInst->getDst()->crossGRF(builder)) {
          // insert a nop
          G4_INST *nopInst = builder.createNop(inst->getOption());
          bb->insertBefore(nextIter, nopInst);
        }
      }

      if (inst->isCall() || inst->isReturn()) {
        inst->setExecSize(kernel.getSimdSize());
      }

      // HW Workaround: for platforms without 64-bit regioning, change send
      // src/dst type from QWord to DWord
      if (builder.no64bitRegioning() && inst->isSend()) {
        G4_DstRegRegion *dst = inst->getDst();
        if (dst != nullptr && dst->getTypeSize() == 8) {
          dst->setType(builder, Type_D);
        }

        G4_Operand *src0 = inst->getSrc(0);
        if (src0 != nullptr && src0->getTypeSize() == 8) {
          src0->asSrcRegRegion()->setType(builder, Type_D);
        }

        if (inst->isSplitSend()) {
          G4_Operand *src1 = inst->getSrc(1);
          if (src1 != nullptr && src1->getTypeSize() == 8) {
            src1->asSrcRegRegion()->setType(builder, Type_D);
          }
        }
      }

      if (inst->isEOT() && VISA_WA_CHECK(builder.getPWaTable(),
                                         WaClearTDRRegBeforeEOTForNonPS)) {
        // insert
        // mov(8) tdr0:uw 0x0:uw {NoMask}
        G4_DstRegRegion *tdrDst =
            builder.createDst(builder.phyregpool.getTDRReg(), 0, 0, 1, Type_UW);
        G4_Imm *src = builder.createImm(0, Type_UW);
        G4_INST *movInst =
            builder.createMov(g4::SIMD8, tdrDst, src,
                              InstOpt_WriteEnable | InstOpt_Switch, false);
        bb->insertBefore(ii, movInst);
      }

      if (inst->isEOT() &&
          VISA_WA_CHECK(builder.getPWaTable(), Wa_14010017096)) {
        // insert "(W) mov(16) acc0.0:f 0x0:f" before EOT
        G4_INST *movInst = builder.createMov(
            g4::SIMD16,
            builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, Type_F),
            builder.createImm(0, Type_F), InstOpt_WriteEnable, false);
        // insert mov before contiguous send, in case that there are instruction
        // combined set on continuous two send
        INST_LIST_ITER insert_point = ii;
        for (; insert_point != bb->begin(); --insert_point)
          if (!(*insert_point)->isSend())
            break;

        if (!(*insert_point)->isEOT())
          ++insert_point;
        bb->insertBefore(insert_point, movInst);
      }

      if (inst->isEOT() &&
          VISA_WA_CHECK(builder.getPWaTable(), Wa_16013338947)) {
        bool hasLegalInstAfterEOT = false;
        for (auto bnext = std::next(ib); bnext != bend; ++bnext) {
          G4_BB *nextBB = *bnext;
          bool found =
              std::any_of(nextBB->begin(), nextBB->end(),
                          [](G4_INST *inst) { return !inst->isLabel(); });
          if (found) {
            hasLegalInstAfterEOT = true;
            break;
          }
        }
        if (!hasLegalInstAfterEOT) {
          G4_INST *nopInst = builder.createNop(InstOpt_NoOpt);
          bb->insertAfter(ii, nopInst);
        }
      }

      if (VISA_WA_CHECK(builder.getPWaTable(), WaResetN0BeforeGatewayMessage) &&
          inst->isSend() && inst->getMsgDesc()->isBarrier()) {
        // mov (1) n0.0 0x0 {Switch}
        G4_DstRegRegion *n0Dst =
            builder.createDst(builder.phyregpool.getN0Reg(), 0, 0, 1, Type_UD);
        auto movInst =
            builder.createMov(g4::SIMD1, n0Dst, builder.createImm(0, Type_UD),
                              InstOpt_WriteEnable | InstOpt_Switch, false);
        bb->insertBefore(ii, movInst);
      }

      linePlaneWA(inst);
      fixSendSrcRegion(inst);
      if (builder.hasMathDpasConflict() && inst->isMath()) {
        INST_LIST_ITER nextIter = ii;
        nextIter++;

        for (int i = 0; i < 5; i++) {
          G4_INST *newInst = inst->cloneInst();
          bb->insertBefore(nextIter, newInst);
        }
        ii = nextIter;
        continue;
      }

      if (VISA_WA_CHECK(builder.getPWaTable(), Wa_22013880840) &&
          builder.getOption(vISA_ALTMode) == true && inst->opcode() == G4_sel &&
          inst->getPredicate() != nullptr && inst->getCondMod() == nullptr &&
          inst->getDst() && IS_TYPE_FLOAT_ALL(inst->getDst()->getType())) {
        auto pred = inst->getPredicate();
        auto movInst1 = builder.createInternalInst(
            builder.duplicateOperand(pred), G4_mov, nullptr,
            inst->getSaturate(), inst->getExecSize(),
            builder.duplicateOperand(inst->getDst()),
            builder.duplicateOperand(inst->getSrc(0)), nullptr,
            inst->getOption());
        bb->insertBefore(ii, movInst1);

        G4_PredState reverse = pred->getState() == PredState_Minus
                                   ? PredState_Plus
                                   : PredState_Minus;
        auto newPred = builder.createPredicate(
            reverse, pred->getBase(), pred->getSubRegOff(), pred->getControl());
        auto movInst2 = builder.createInternalInst(
            newPred, G4_mov, nullptr, inst->getSaturate(), inst->getExecSize(),
            builder.duplicateOperand(inst->getDst()),
            builder.duplicateOperand(inst->getSrc(1)), nullptr,
            inst->getOption());
        *ii = movInst2;
        inst->removeAllDefs();
      }

      if (builder.kernel.getNumRegTotal() == 256 && inst->isEOT() &&
          VISA_WA_CHECK(builder.getPWaTable(), Wa_14016880151)) {
        INST_LIST_ITER preIter = std::prev(ii);
        if (preIter != ii) {
          G4_INST *preInst = (*preIter);
          if (preInst->isAtomicInst()) {
            insertDummyCsel(bb, preIter, false);
          } else {
            insertDummyCsel(bb, ii, false);
          }
        }
      }

      if (builder.needBarrierWA() && inst->isBarrierWAIntrinsic()) {
        applyBarrierWA(ii, bb);
      }

      if (builder.needBarrierWA() && inst->isNamedBarrierWAIntrinsic()) {
        applyNamedBarrierWA(ii, bb);
      }

      if (inst->isIEEEExceptionTrap())
        expandIEEEExceptionTrap(ii, bb);

      // Double up every TGM fence instruction if fenceOp is not
      // LSC_FENCE_OP_NONE
      if (builder.needTGMDoubleFenceWA() && inst->isSend() &&
          inst->asSendInst()->isFence() &&
          inst->asSendInst()->getMsgDesc()->getSFID() == SFID::TGM &&
          inst->asSendInst()->getMsgDescRaw()->getLscFenceOp() !=
              LSC_FENCE_OP_NONE)
        bb->insertBefore(ii, inst->cloneInst());

      ii++;
    }
  }

  if (VISA_WA_CHECK(builder.getPWaTable(), WaClearArfDependenciesBeforeEot)) {
    clearARFDependencies();
  }
  if (VISA_WA_CHECK(builder.getPWaTable(), Wa_2201674230)) {
    clearSendDependencies();
  }

  if (builder.hasMulMacRSIssue()) {
    mulMacRSWA();
  }

  if (builder.needResetA0forVxHA0()) {
    // reset a0 to 0 at the beginning of a shader.
    // The goal of this initialization is to make sure that there is no
    // garbage values in the address register for inactive simd lanes.
    // With indirect addressing HW requires that there is no
    // out-of-bounds access even on inactive simd lanes.

    // Note: this initialization doesn't cover scenarios where the
    // address register is used in a send descriptor and later used in
    // indirect addressing.
    resetA0();
  }

  if (builder.getOption(vISA_setA0toTdrForSendc)) {
    // set A0 to tdr0 before sendc/sendsc. TGL WA
    setA0toTdrForSendc();
  }

  if (builder.needReplaceIndirectCallWithJmpi() &&
      kernel.getBoolKernelAttr(Attributes::ATTR_Extern)) {
    // jmpi WA can't properly work on platforms with SWSB. We didn't re-caculate
    // the jump offset after swsb insertion.
    vASSERT(!builder.hasSWSB());
    // replace ret in the external functions with jmpi. That we will
    // also replace the call with jmpi in
    // Optimizer::expandIndirectCallWithRegTarget
    replaceRetWithJmpi();
  }

  if (!builder.supportCallaRegSrc() && kernel.hasIndirectCall()) {
    // If the indirect call has regiser src0, the register must be a
    // ip-based address of the call target. Insert instructions before call to
    // calculate the relative offset from call to the target
    expandIndirectCallWithRegTarget();
  }

  if (builder.hasFPU0ReadSuppressionIssue()) {
    fixReadSuppressioninFPU0();
  }
}

// When destination is an address register the following apply:
// Destination must not span across the lower to upper 8 dword
// boundary of the register.
// Fix this restriction after RA instead of HWConformity just because
// RA(spill/fill, A0 save/restore) would generate such instructions.
void Optimizer::fixDirectAddrBoundOnDst() {
  HWConformity hwConf(builder, kernel);
  for (auto bb : kernel.fg) {
    for (auto it = bb->begin(), ie = bb->end(); it != ie; ++it) {
      G4_INST *inst = *it;
      G4_DstRegRegion *dst = inst->getDst();
      if (dst && !dst->isNullReg() &&
          dst->getRegAccess() == Direct && dst->getTopDcl() &&
          dst->getTopDcl()->getRegVar()->isAddress()) {
        G4_Declare *dcl = dst->getTopDcl();
        if (dcl->getTotalElems() > Eight_Word) {
          if (dcl->getSubRegAlign() < Sixteen_Word)
            dcl->setSubRegAlign(Sixteen_Word);
        } else if (dcl->getTotalElems() > Four_Word) {
          if (dcl->getSubRegAlign() < Eight_Word)
            dcl->setSubRegAlign(Eight_Word);
        } else if (dcl->getTotalElems() > Any) {
          if (dcl->getSubRegAlign() < Four_Word)
            dcl->setSubRegAlign(Four_Word);
        }
        if (((dst->getSubRegOff() + inst->getExecSize() - 1) / 16 !=
                (dst->getSubRegOff() / 16)) ||
            inst->getExecSize() == g4::SIMD32) {
          hwConf.evenlySplitInst(it, bb, /*checkOverlap*/ false);
        }
      }
    }
  }
}

static bool retires(G4_Operand *Opnd, G4_INST *SI) {
  vASSERT(SI);
  const IR_Builder &builder = SI->getBuilder();
  vASSERT(Opnd && Opnd->isGreg());
  unsigned LB = Opnd->getLinearizedStart() / builder.numEltPerGRF<Type_UB>();
  unsigned RB = Opnd->getLinearizedEnd() / builder.numEltPerGRF<Type_UB>();

  auto overlaps = [=, &builder](G4_Operand *A) {
    if (A == nullptr || A->isNullReg() || !A->isGreg())
      return false;
    unsigned LB1 = A->getLinearizedStart() / builder.numEltPerGRF<Type_UB>();
    unsigned RB1 = A->getLinearizedEnd() / builder.numEltPerGRF<Type_UB>();
    return (RB >= LB1 && RB1 >= LB);
  };

  // RAW or WAW
  if (overlaps(SI->getDst()))
    return true;

  if (Opnd->isSrcRegRegion())
    return false;

  // WAR.
  if (overlaps(SI->getSrc(0)))
    return true;
  if (SI->isSplitSend() && overlaps(SI->getSrc(1)))
    return true;

  // Do not retire this send.
  return false;
}

// Emit a self-move to retire this send.
static G4_INST *emitRetiringMov(IR_Builder &builder, G4_BB *BB, G4_INST *SI,
                                INST_LIST_ITER InsertBefore) {
  vASSERT(SI && SI->isSend());
  G4_Operand *Src0 = SI->getSrc(0);

  unsigned RegNum =
      Src0->getLinearizedStart() / builder.numEltPerGRF<Type_UB>();
  G4_Declare *Dcl = builder.createTempVar(16, Type_F, Any);
  Dcl->getRegVar()->setPhyReg(builder.phyregpool.getGreg(RegNum), 0);

  G4_DstRegRegion *MovDst =
      builder.createDst(Dcl->getRegVar(), 0, 0, 1, Type_F);
  G4_SrcRegRegion *MovSrc = builder.createSrc(
      Dcl->getRegVar(), 0, 0, builder.getRegionStride1(), Type_F);
  G4_INST *MovInst = builder.createMov(g4::SIMD8, MovDst, MovSrc,
                                       InstOpt_M0 | InstOpt_WriteEnable, false);
  BB->insertBefore(InsertBefore, MovInst);
  return MovInst;
}

// Use this instruction to retire live sends.
static void retireSends(std::vector<G4_INST *> &LiveSends, G4_INST *Inst) {
  if (LiveSends.empty())
    return;

  // Predicated instructions may not retire a send.
  if (Inst->getPredicate() != nullptr && Inst->opcode() != G4_sel)
    return;

  // Collect operands for dependency checking.
  std::vector<G4_Operand *> Opnds;
  if (G4_DstRegRegion *Dst = Inst->getDst()) {
    if (!Dst->isNullReg() && !Dst->isIndirect() && Dst->isGreg())
      Opnds.push_back(Dst);
  }
  for (int i = 0; i < Inst->getNumSrc(); ++i) {
    G4_Operand *Opnd = Inst->getSrc(i);
    if (Opnd == nullptr || !Opnd->isSrcRegRegion() || Opnd->isNullReg())
      continue;
    G4_SrcRegRegion *Src = Opnd->asSrcRegRegion();
    if (!Src->isIndirect() && Src->isGreg())
      Opnds.push_back(Opnd);
  }

  // WRA, RAW or WAW dependency retires a live send.
  bool Changed = false;
  for (auto Opnd : Opnds) {
    for (auto &SI : LiveSends) {
      if (SI && retires(Opnd, SI)) {
        SI = nullptr;
        Changed = true;
      }
    }
  }
  // Remove nullptr values when there are changes.
  if (Changed) {
    auto Iter =
        std::remove(LiveSends.begin(), LiveSends.end(), (G4_INST *)nullptr);
    LiveSends.erase(Iter, LiveSends.end());
  }
}

// Limit the number of live sends and clear all sends at the end of a block.
void Optimizer::clearSendDependencies() {
  for (auto BB : fg) {
    // Live send instructions. This vector will only have MAX_SENDS
    // or less instructions.
    const unsigned MAX_SENDS = 3;
    std::vector<G4_INST *> LiveSends;

    for (auto I = BB->begin(); I != BB->end(); /*empty*/) {
      auto CurI = I++;
      G4_INST *Inst = *CurI;

      // Try to retire live sends.
      retireSends(LiveSends, Inst);
      if (!Inst->isSend())
        continue;

      // This is a send.
      if (LiveSends.size() >= MAX_SENDS) {
        // OK, too many live sends. Retire the earliest live send.
        G4_INST *SI = LiveSends.front();
        G4_INST *MovInst = emitRetiringMov(builder, BB, SI, CurI);
        retireSends(LiveSends, MovInst);
        vASSERT(LiveSends.size() < MAX_SENDS);
      }

      // If this is EOT and send queue is not full, then nothing to do.
      // Otherwise a new send becomes live.
      if (Inst->isEOT())
        LiveSends.clear();
      else
        LiveSends.push_back(Inst);
    }

    // Retire remainig live sends in this block, if any.
    for (auto SI : LiveSends) {
      vASSERT(SI && SI->isSend());
      auto InsertBefore = BB->end();
      G4_INST *LastInst = BB->back();
      if (LastInst->isFlowControl())
        InsertBefore = std::prev(InsertBefore);
      emitRetiringMov(builder, BB, SI, InsertBefore);
    }
  }
}
