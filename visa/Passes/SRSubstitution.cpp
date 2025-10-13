/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SRSubstitution.hpp"
#include <cmath>

using namespace vISA;

static bool regSortCompare(regMap map1, regMap map2) {
  if (map1.dstReg < map2.dstReg) {
    return true;
  }

  return false;
}

static bool regSortCompareAfterRA(regMapBRA map1, regMapBRA map2) {
  if (map1.opndNum < map2.opndNum) {
    return true;
  } else if (map1.opndNum > map2.opndNum) {
    return false;
  }

  if (map1.offset < map2.offset) {
    return true;
  } else if (map1.offset > map2.offset) {
    return false;
  }

  if (map1.inst->getLocalId() < map2.inst->getLocalId()) {
    return true;
  }

  return false;
}

void changeToIndirectSend(G4_INST *inst, G4_Declare *s0Var, int totalRegs,
                          IR_Builder &builder, bool isLargeGRF) {
  // Change the send instruction to sendi
  G4_InstSend *Send = inst->asSendInst();
    G4_SendDescRaw *desc = Send->getMsgDescRaw();
    desc->setExtMessageLength(0);
    G4_Operand *msgDesc = inst->getSrc(2);

    uint32_t descImm = (uint32_t)msgDesc->asImm()->getImm();
    descImm &= 0xE1FFFFFF; // clear bit 25:28
    descImm |= totalRegs << 25;
    G4_Imm *msgDescImm = builder.createImm(descImm, Type_UD);
    inst->setSrc(msgDescImm, 2);

  // Replace source 0 with scalar register
  G4_SrcRegRegion *headerOpnd =
      builder.createSrcRegRegion(Mod_src_undef, IndirGRF, s0Var->getRegVar(), 0,
                                 0, builder.getRegionScalar(), Type_UB);
  // Replace source 1 with null.
  G4_SrcRegRegion *payloadToUse = builder.createNullSrc(Type_UD);

  inst->setSrc(headerOpnd, 0);
  inst->setSrc(payloadToUse, 1);
}

// Check if current instruction is the candidate of sendi.
// Recorded as candidate.
bool SRSubPass::isSRCandidate(G4_INST *inst, regCandidates &dstSrcRegs) {
  if (!inst->isSend()) {
    return false;
  }

  // The payload size is less than 2 GRFs, no need to use sendi
  if (inst->getMsgDesc()->getSrc1LenRegs() < 2) {
    return false;
  }

  int movInstNum = 0;
  int32_t firstDefID = 0x7FFFFFFF; // the ID of the first instruction define the
                                   // payload of the send
  std::list<USE_DEF_NODE> SRCandidate;
  SRCandidate.clear();
  bool src0Out = false;
  for (auto I = inst->def_begin(), E = inst->def_end(); I != E; ++I) {
    auto &&def = *I;

    if (def.second != Opnd_src1 && def.second != Opnd_src0) {
      continue;
    }

    if (def.first->opcode() == G4_mov) {
      movInstNum++;
    }

    // The instruction is only used for payload preparation.
    if (def.first->use_size() != 1) {
      return false;
    }

    // It's not global define
    if (kernel.fg.globalOpndHT.isOpndGlobal(def.first->getDst())) {
      if (def.second == Opnd_src1) {
        return false;
      } else { //src0
        src0Out = true;
        continue;
      }
    }

    SRCandidate.push_back(def);
    firstDefID = std::min(firstDefID, def.first->getLocalId());
  }

  // Check if there are enough mov instructions to be removed.
  if (movInstNum < 2) {
    return false;
  }

  unsigned short GRFSize = builder.getGRFSize();
  unsigned short GRFNum = builder.kernel.getNumRegTotal();
  BitSet definedReg(GRFNum, false);
  for (auto I = SRCandidate.begin(), E = SRCandidate.end(); I != E; ++I) {
    G4_INST *cInst = (*I).first;
    Gen4_Operand_Number opndNum = (*I).second;
    G4_Operand *dst = cInst->getDst();
    // GRF size aligned
    if ((dst->getLinearizedEnd() - dst->getLinearizedStart() + 1) % GRFSize !=
        0) {
      return false;
    }

    for (unsigned i = dst->getLinearizedStart() / GRFSize; i < dst->getLinearizedEnd() / GRFSize; i ++ ) {
      // r4, r5, r6, r7 are multiple defined
      // (W) send.ugm(1 | M0) r4 r36 null:0 0x85000000:a0.2 0x4240F500 {$18} //
      // wr:1+0, rd:4; load.ugm.d32x64t.a32.flat[A-0x7B000] fill from
      // offset[40*64] of COAL_FILL_3164; $954
      // mov(32 | M0) r4 .0 < 1 > : ud r10 .0 < 1;1,0> : ud{Compacted, $18.dst}
      // mov(32 | M0) r6 .0 < 1 > : ud r8 .0 < 1;1,0> :  ud{Compacted}
      // (W) send.ugm(1 | M0) null r36 r4 : 4 0x85000000:a0.2 0x4200F504 {I @1,
      // $19} // wr:1+4, rd:0; store.ugm.d32x64t.a32.flat[A-0x7B000]   spill to
      // offset[40*64] of COAL_FILL_3164; $954
      if (definedReg.isSet(i) && opndNum == Opnd_src1) { //No multiple define in src1
        return false;
      } else {
        definedReg.set(i, true);
      }
    }
    // The payload preparation instruction may be mov,or other instsruction
    // For mov instruction, use the source register and remove the mov
    // instruction For non-mov instruction, use the dst register directly
    // without remove the instruction
    //  or (8)              r15.0 < 1 > :d  r114.0 < 1; 1, 0 > : d  r2.0 < 1; 1, 0 > : d
    //  mov(8)              r13.0 < 1 > :f  r21.0 < 1; 1, 0 > : f
    //  mov(8)              r14.0 < 1 > :f  r18.0 < 1; 1, 0 > : f
    //  mov(8)              r16.0 < 1 > :f  r55.0 < 1; 1, 0 > : f
    //  mov(8)              r12.0 < 1 > :d  r111.0 < 1; 1, 0 > : d
    //  sends(8)            null : ud  r112  r12 0x14c : ud 0x2035091 :
    //  ud //typed surface write, resLen=0, msgLen=1, extMsgLen=5
    if (cInst->opcode() == G4_mov) {
      G4_Operand *src = cInst->getSrc(0);
      unsigned short dstRegLB = dst->getLinearizedStart() / GRFSize;
      unsigned short dstRegRB = dst->getLinearizedEnd() / GRFSize;
      unsigned short srcRegLB = src->getLinearizedStart() / GRFSize;
      unsigned short srcRegRB = src->getLinearizedEnd() / GRFSize;

      // GRF aligned
      if (dst->getLinearizedStart() % GRFSize != 0 ||
          src->getLinearizedStart() % GRFSize != 0) {
        return false;
      }

      // In case src region is not consistent with dst region
      if (srcRegRB - srcRegLB != dstRegRB - dstRegLB) {
        return false;
      }
    } else {
      // GRF aligned
      if (dst->getLinearizedStart() % GRFSize != 0) {
        return false;
      }
    }
  }

  for (auto I = SRCandidate.begin(), E = SRCandidate.end(); I != E; ++I) {
    G4_INST *cInst = (*I).first;
    Gen4_Operand_Number opndNum = (*I).second;

    if (src0Out && opndNum == Opnd_src0) {
      continue;
    }

    G4_Operand *dst = cInst->getDst();
    unsigned short dstRegLB = dst->getLinearizedStart() / GRFSize;
    unsigned short dstRegRB = dst->getLinearizedEnd() / GRFSize;

    if (cInst->opcode() == G4_mov) {
      G4_Operand *src = cInst->getSrc(0);
      unsigned short srcRegLB = src->getLinearizedStart() / GRFSize;
      for (unsigned short reg = dstRegLB; reg <= dstRegRB; reg++) {
        regMap regPair(cInst->getLocalId(), reg,
                       srcRegLB + reg - dstRegLB);
        dstSrcRegs.dstSrcMap.push_back(regPair);
        if (opndNum == Opnd_src0)
        {
            dstSrcRegs.includeSrc0 = true;
        }
      }
    } else // For none-mov instruction, we will use the dst directly,
           // instruction will not be erased.
    {
      for (unsigned short reg = dstRegLB; reg <= dstRegRB; reg++) {
        regMap regPair(cInst->getLocalId(), reg, reg);
        dstSrcRegs.dstSrcMap.push_back(regPair);
      }
    }
  }

  // Record the smallest local ID of the define instructions, for the valid
  // candidate scan.
  dstSrcRegs.firstDefID = firstDefID;

  // Sort according to the register order in the original payload
  std::sort(dstSrcRegs.dstSrcMap.begin(), dstSrcRegs.dstSrcMap.end(),
            regSortCompare);

  return true;
}

// Replace the send instruction with the payload of
// Insert the scalar register intialization mov instructions.
bool SRSubPass::replaceWithSendi(G4_BB *bb, INST_LIST_ITER instIter,
                                 std::vector<regMap> &dstSrcRegs, bool src0Mov) {
  G4_INST *inst = *instIter;
  int numRows = dstSrcRegs.size();
  int totalRegs = numRows;
  vASSERT(totalRegs <= MAXIMAL_S0_SRC0_GRF_LENGTH);
  uint64_t value = 0;
  uint64_t value1 = 0;
  int value_size = 0;
  int value1_size = 0;

  if (!src0Mov) {
    G4_Operand *src0 = inst->getSrc(0);
    unsigned short GRFSize = builder.getGRFSize();
    unsigned short src0RegLB = src0->getLinearizedStart() / GRFSize;
    unsigned short src0RegRB = src0->getLinearizedEnd() / GRFSize;
    totalRegs += src0RegRB - src0RegLB + 1;

    // Original src0
    for (int i = 0; i < (src0RegRB - src0RegLB + 1); i++, value_size++) {
      value |= ((uint64_t)src0RegLB + i) << (8 * i);
    }
  }

  // Plusing the src1, totally 8 element at most
  int i = 0;
  for (; i < numRows && value_size < 8; i++, value_size++) {
    value |= ((uint64_t)dstSrcRegs[i].srcReg) << 8 * value_size;
  }
  vASSERT(value_size <= 8);

  // The remaining data. May more than 8 GRFs, so will need two mov instructions
  for (int j = i; j < numRows; j++, value1_size++) {
    value1 |= ((uint64_t)dstSrcRegs[j].srcReg) << 8 * (j - i);
  }

  // Initialize the scalar registers.
  uint16_t UQNum = totalRegs > (TypeSize(Type_UQ) / TypeSize(Type_UB)) ? 2 : 1;
  G4_Declare *s0Var = builder.createScalar(UQNum, "S0_");
  s0Var->getRegVar()->setPhyReg(builder.phyregpool.getScalarReg(), 0);
  G4_Imm *src = builder.createImm(value, Type_UQ);
  G4_DstRegRegion *dst =
      builder.createDst(s0Var->getRegVar(), 0, 0, 1, Type_UQ);
  G4_INST *movInst =
      builder.createMov(g4::SIMD1, dst, src, InstOpt_WriteEnable, false);
  bb->insertBefore(instIter, movInst);

  // Second initialization if more than 8 GRFs
  if (value1_size) {
    G4_Imm *src = builder.createImm(value1, Type_UQ);
    G4_DstRegRegion *dst =
        builder.createDst(s0Var->getRegVar(), 0, 1, 1, Type_UQ);
    G4_INST *movInst =
        builder.createMov(g4::SIMD1, dst, src, InstOpt_WriteEnable, false);
    bb->insertBefore(instIter, movInst);
  }

  changeToIndirectSend(inst, s0Var, totalRegs, builder, false);

  return true;
}

// Subsititue with scalar regsiter
void SRSubPass::SRSub(G4_BB *bb) {
  bb->resetLocalIds();

  class CmpFirstDef {
  public:
    bool operator()(G4_INST *first, G4_INST *second) const {
      return first->getLocalId() < second->getLocalId();
    }
  };
  std::map<G4_INST *, regCandidates, CmpFirstDef> candidates;
  std::map<G4_INST *, regCandidates, CmpFirstDef>::iterator candidatesIt;

  INST_LIST_ITER ii = bb->begin(), iend(bb->end());
  while (ii != iend) {
    G4_INST *inst = *ii;

    regCandidates dstSrcRegs;
    if (!isSRCandidate(inst, dstSrcRegs)) {
      ii++;
      continue;
    }
    candidates.insert(std::make_pair(inst, dstSrcRegs));
    ii++;
  }

  // If the registser is redefined before the send, the substitution needs be
  // cancelled. Such as r9 in following instructions, which is redefined in sel
  // instruction.
  // mov(8)             r6.0< 1 > :f  r9.0 < 1; 1, 0 > : f
  // sel(8) (ge)f0.0    r9.0< 1 > :f  r57.0 < 1; 1, 0 > : f
  // mov(8)             r3.0< 1 > :f  r79.0 < 1; 1, 0 > : f
  // mov(8)             r4.0 < 1 > : f  r27.0 < 1; 1, 0 > : f
  // mov(8)             r5.0 < 1 > : f  r77.0 < 1; 1, 0 > : f
  // mov(8)             r2.0 < 1 > : d  r111.0 < 1; 1, 0 > : d
  // sends(8)           null : ud r112 r2 0x14c : ud 0x2035092 : ud //typed
  // surface write, resLen=0, msgLen=1, extMsgLen=5
  INST_LIST_RITER ri = bb->rbegin(), rend(bb->rend());
  while (ri != rend) {
    G4_INST *inst = *ri;
    candidatesIt = candidates.find(inst);
    if (candidatesIt != candidates.end()) {
      bool overwrite = false;
      INST_LIST_RITER scan_ri = ri;
      scan_ri++;
      G4_INST *rInst = *scan_ri;
      while (rInst->getLocalId() > candidates[inst].firstDefID) {
        G4_Operand *dst = rInst->getDst();
        if (dst && !dst->isNullReg()) {
          G4_VarBase *base = dst->getBase();
          G4_VarBase *phyReg =
              (base->isRegVar()) ? base->asRegVar()->getPhyReg() : base;

          if (phyReg->getKind() == G4_VarBase::VK_phyGReg) {
            // If the register reuse happened in the mov instruction which will
            // be removed. Or the instruction whose dst will be accessed through
            // s0. It's not real reuse. Such as the r8 in following
            // instructions.
            //clang-format off
            // mov(8)  r9.0 <1>:f  r8.0<1; 1, 0>:f
            // mov(8)  r10.0<1>:f  r3.0<8; 8, 1>:f
            // mov(8)  r11.0<1>:f  r3.0<8; 8, 1>:f
            // mov(8)  r7.0 <1>:d  r5.0<1; 1, 0>:d
            // mov(8)  r8.0 <1>:f  r3.0<1; 1, 0>:f
            // sends(8) null:ud r112 r7 0x14c : ud 0x2035093 : ud // typed surface write, resLen=0, msgLen=1, extMsgLen=5
            //clang-format on
            bool sameInst = false;
            for (int i = 0; i < (int)candidates[inst].dstSrcMap.size(); i++) {
              if (rInst->getLocalId() ==
                  candidates[inst].dstSrcMap[i].localID) {
                sameInst = true;
                break;
              }
            }

            if (!sameInst) {
              unsigned short GRFSize = builder.getGRFSize();
              G4_Operand *dst = rInst->getDst();
              unsigned short dstRegLB = dst->getLinearizedStart() / GRFSize;
              unsigned short dstRegRB = dst->getLinearizedEnd() / GRFSize;
              for (int i = 0; i < (int)candidates[inst].dstSrcMap.size(); i++) {
                int srcReg = candidates[inst].dstSrcMap[i].srcReg;
                if (!(srcReg < dstRegLB || srcReg > dstRegRB)) {
                  // Register is reused.
                  overwrite = true;
                  break;
                }
              }
            }
          }
        }
        scan_ri++;
        if (scan_ri == rend) {
          break;
        }
        rInst = *scan_ri;
      }
      if (overwrite) {
        candidates.erase(candidatesIt);
      }
    }

    ri++;
  }

  for (candidatesIt = candidates.begin(); candidatesIt != candidates.end();
       candidatesIt++) {
    G4_INST *inst = candidatesIt->first;

    for (auto I = inst->def_begin(), E = inst->def_end(); I != E; ++I) {
      auto &&def = *I;
      if (def.first->opcode() == G4_mov) // Only mov instruction will be erased
      {
        def.first->markDead();
      }
    }
  }

  // Replace the send instruction with sendi
  // Remove the mov instructions that marked as dead
  INST_LIST_ITER iter;
  for (iter = bb->begin(); iter != bb->end();) {
    G4_INST *inst = *iter;
    INST_LIST_ITER curIter = iter;
    iter++;

    candidatesIt = candidates.find(inst);
    if (candidatesIt != candidates.end()) {
      replaceWithSendi(bb, curIter, candidates[inst].dstSrcMap,
                       candidates[inst].includeSrc0);
    }
    if (inst->isDead()) {
      bb->erase(curIter);
    }
    // FIXME: after replacing with sendi and scalar register mov, the local data
    // flow analysis need be able to capture the change.
  }
}

// Check if current instruction is the candidate of sendi.
// Recorded as candidate.
bool SRSubPassAfterRA::isSRCandidateAfterRA(G4_INST *inst,
                                            regCandidatesBRA &dstSrcRegs) {
  if (!inst->isSend()) {
    return false;
  }

  if (!inst->asSendInst()->getMsgDescRaw()) {
    return false;
  }

  if (!inst->isSplitSend()) {
    return false;
  }

  G4_Operand *msgDesc = inst->getSrc(2);
  if (!inst->asSendInst()->getMsgDesc() && !(msgDesc && msgDesc->isImm())) {
    return false;
  }

  // The payload size is less than 2 GRFs, no need to use sendi
  if ((inst->getMsgDesc()->getSrc1LenRegs() +
       inst->getMsgDesc()->getSrc0LenRegs()) <= 2) {
    return false;
  }

  if ((inst->getMsgDesc()->getSrc1LenRegs() +
       inst->getMsgDesc()->getSrc0LenRegs()) > MAXIMAL_S0_SRC0_GRF_LENGTH) {
    return false;
  }

  // The size of LSC src0 and src1 may not be GRF aligned.
  if (inst->getMsgDesc()->getSrc1LenBytes() % builder.getGRFSize() != 0 ||
      inst->getMsgDesc()->getSrc0LenBytes() % builder.getGRFSize() != 0) {
    return false;
  }

  if (builder.getuint32Option(vISA_EnableGatherWithImmPreRA) ==
      INDIRECT_TYPE::ALWAYS_S0) {
    return true;
  }

  SFID funcID = inst->asSendInst()->getMsgDesc()->getSFID();
  if (builder.getuint32Option(vISA_EnableGatherWithImmPreRA) ==
          INDIRECT_TYPE::SAMPLER_MSG_ONLY &&
      funcID != SFID::SAMPLER) {
    return false;
  }


  int movInstNum = 0;
  int32_t firstDefID = 0x7FFFFFFF; // the ID of the first instruction define the
  std::vector<std::pair<Gen4_Operand_Number, unsigned>> notRemoveableMap;
  std::vector<G4_INST *> immMovs;
  for (auto I = inst->def_begin(), E = inst->def_end(); I != E; ++I) {
    auto &&def = *I;

    // Only src0 and src1 matter
    if (def.second != Opnd_src1 && def.second != Opnd_src0) {
      continue;
    }

    G4_INST *movInst = def.first;
    auto isRemoveAble = [this](G4_INST *i) {
      if (i->opcode() != G4_mov) {
        return false;
      }

      if (i->getPredicate() || i->getCondMod() || i->getSaturate()) {
        return false;
      }

      // The instruction is only used for payload preparation.
      if (i->use_size() != 1) {
        return false;
      }

      G4_DstRegRegion *dstRgn = i->getDst();
      G4_Operand *src = i->getSrc(0);

      if (!src->isSrcRegRegion()) {
        return false;
      }

      // Not GRF source
      if (!(src->getBase()->isRegVar() &&
            (src->getBase()->asRegVar()->getDeclare()->getRegFile() == G4_GRF ||
             src->getBase()->asRegVar()->getDeclare()->getRegFile() == G4_INPUT))) {
        return false;
      }

      // The src region is scalar
      if (src->isScalarSrc()) {
        return false;
      }

      G4_SrcRegRegion *srcRgn = src->asSrcRegRegion();

      if (srcRgn->hasModifier()) {
        return false;
      }

      //dst GRF aligned and contigous
      if (dstRgn->getSubRegOff() ||
          dstRgn->getHorzStride() != 1) {
        return false;
      }

      // src GRF aligned and contigous
      if (srcRgn->getSubRegOff() ||
          !srcRgn->getRegion()->isContiguous(i->getExecSize())) {
        return false;
      }

      // The src region is not packed
      if (!srcRgn->getRegion()->isPackedRegion()) {
        return false;
      }

      //No type conversion
      if (dstRgn->getType() != src->getType()) {
        return false;
      }

      // If the destination operand size is less than 1 GRF
      if ((dstRgn->getLinearizedEnd() - dstRgn->getLinearizedStart() + 1) <
          builder.getGRFSize()) {
        return false;
      }

      // If the source operand size is less than 1 GRF
      if ((src->getLinearizedEnd() - src->getLinearizedStart() + 1) <
          builder.getGRFSize()) {
        return false;
      }

      //GRF Alignment with physical register assigned
      if (dstRgn->getLinearizedStart() % builder.getGRFSize() != 0 ||
          src->getLinearizedStart() % builder.getGRFSize() != 0) {
        return false;
      }

      // Move to self instruction will be removed by following pass. So cannot
      // be counted.
      if (dstRgn->getLinearizedStart() == src->getLinearizedStart()) {
        return false;
      }

      // It's not global define
      if (!(builder.getIsKernel() && kernel.fg.getNumBB() == 1)) {
        if (kernel.fg.globalOpndHT.isOpndGlobal(dstRgn) && !dstRgn->getTopDcl()->getIsBBLocal()) {
          return false;
        }
      }

      return true;
    };

    // mov (16)             r81.0<1>:f  0x8:f // $52:&54:
    // mov (16|M16)         r89.0<1>:f  0x8:f // $53:&55:
    // mov (16)             r82.0<1>:f  0x0:f // $54:&56:
    // mov (16|M16)         r90.0<1>:f  0x0:f // $55:&57:
    // mov (16)             r83.0<1>:f  0x0:f // $56:&58:
    // mov (16|M16)         r91.0<1>:f  0x0:f // $57:&59:
    // mov (16)             r84.0<1>:f  0x0:f // $58:&60:
    // mov (16|M16)         r92.0<1>:f  0x0:f // $59:&61:
    // mov (16)             r85.0<1>:f  0x0:f // $60:&62:
    // mov (16|M16)         r93.0<1>:f  0x0:f // $61:&63:
    // mov (16)             r86.0<1>:f  0x0:f // $62:&64:
    // mov (16|M16)         r94.0<1>:f  0x0:f // $63:&65:
    // mov (16)             r87.0<1>:f  0x0:f // $64:&66:
    // mov (16|M16)         r95.0<1>:f  0x0:f // $65:&67:
    // mov (16)             r88.0<1>:f  0x0:f // $66:&68:
    // mov (16|M16)         r96.0<1>:f  0x0:f // $67:&69:
    // ==>
    // mov (16)             r81.0<1>:f  0x8:f // $52:&54:
    // mov (16|M16)         r89.0<1>:f  0x8:f // $53:&55:
    // mov (16)             r82.0<1>:f  0x0:f // $54:&56:
    // mov (16|M16)         r90.0<1>:f  0x0:f // $55:&57:
    //
    // Reuse r81, r89, r82, r90 in the gather send
    auto getRemoveableImm = [this](G4_INST *inst,
                                   std::vector<G4_INST *> &immMovs) {
      // The instruction is only used for payload preparation.
      if (inst->use_size() != 1) {
        return (G4_INST *)nullptr;
      }

      if (inst->opcode() != G4_mov) {
        return (G4_INST *)nullptr;
      }

      G4_DstRegRegion *dst = inst->getDst();
      // dst GRF aligned and contigous
      if (dst->getSubRegOff() || dst->getHorzStride() != 1) {
        return (G4_INST *)nullptr;
      }

      if (kernel.fg.globalOpndHT.isOpndGlobal(dst)) {
        return (G4_INST *)nullptr;
      }

      // GRF Alignment with physical register assigned
      if (dst->getLinearizedStart() % builder.getGRFSize() != 0) {
        return (G4_INST *)nullptr;
      }

      // If the destination operand size is less than 1 GRF
      if ((dst->getLinearizedEnd() - dst->getLinearizedStart() + 1) <
          builder.getGRFSize()) {
        return (G4_INST *)nullptr;
      }

      G4_Operand *src = inst->getSrc(0);
      int64_t imm = src->asImm()->getImm();
      for (size_t i = 0; i < immMovs.size(); i++) {
        G4_INST *imov = immMovs[i];
        G4_Operand *isrc = imov->getSrc(0);
        int64_t iimm = isrc->asImm()->getImm();
        if (imm == iimm &&
            src->getType() == isrc->getType() && // Same value and same type
            inst->getDst()->getType() ==
                imov->getDst()->getType() && // Same dst type
            inst->getDst()->asDstRegRegion()->getHorzStride() ==
                imov->getDst()
                    ->asDstRegRegion()
                    ->getHorzStride() &&                  // Same region
            inst->getExecSize() == imov->getExecSize() && // Same execution size
            inst->getMaskOffset() ==
                imov->getMaskOffset()) { // Same mask offset
          return imov;
        }
      }
      immMovs.push_back(inst);

      return (G4_INST *)nullptr;
    };

    // The source opndNum of send instruction which was defined
    Gen4_Operand_Number opndNum = (*I).second;
    //if opndNum + offset is defined multiple times, cannobe be removed
    G4_Operand *dst = movInst->getDst();
    // The startOffset is the offset to the declare
    unsigned startOffset = dst->getLeftBound() / builder.getGRFSize();
    unsigned dstSize = (dst->getLinearizedEnd() - dst->getLinearizedStart() +
                        builder.getGRFSize() - 1) /
                       builder.getGRFSize();

    if (isRemoveAble(movInst)) {
      auto iter = std::find_if(
          dstSrcRegs.dstSrcMap.begin(), dstSrcRegs.dstSrcMap.end(),
          [opndNum, dst](regMapBRA regmap) {
                         return regmap.opndNum == opndNum &&
                                !((regmap.inst->getDst()->getLinearizedStart() >
                                     dst->getLinearizedEnd()) ||
                                 (dst->getLinearizedStart() >
                                     regmap.inst->getDst()->getLinearizedEnd()));
          });
      // if multiple defined, cannot be removed
      if (iter != dstSrcRegs.dstSrcMap.end()) {
        for (unsigned offset = startOffset; offset < dstSize; offset++) {
          notRemoveableMap.push_back(std::make_pair(opndNum, offset));
        }
      } else {
        G4_Operand *src = movInst->getSrc(0);
        regMapBRA regPair(movInst, opndNum, startOffset, src); // mov source
        dstSrcRegs.dstSrcMap.push_back(regPair);
        firstDefID = std::min(firstDefID, def.first->getLocalId());
        movInstNum++;
      }
    } else {
      if (movInst->opcode() == G4_mov && movInst->getSrc(0) &&
          movInst->getSrc(0)->isImm()) {
        // Check if there is mov instruction with same imm value
        G4_INST *lvnMov = getRemoveableImm(movInst, immMovs);

        if (lvnMov) {
          // The offset is the offset of original dst, which is used to identify
          // the original register used in send.
          // The opndNum is the opndNum of send.
          regMapBRA regPair(movInst, opndNum, startOffset,
                            lvnMov->getDst()); // the lvn mov dst can be reused
          dstSrcRegs.dstSrcMap.push_back(regPair);
          firstDefID = std::min(firstDefID, def.first->getLocalId());
          movInstNum++;
          continue;
        }
      }
      for (unsigned offset = startOffset; offset < dstSize; offset++) {
        notRemoveableMap.push_back(std::make_pair(opndNum, offset));
      }
    }
  }

  // There is any none removeable offset, the offset define move cannot
  // be removed.
  std::vector<regMapBRA>::iterator dstSrcRegsIter;
  for (dstSrcRegsIter = dstSrcRegs.dstSrcMap.begin();
       dstSrcRegsIter != dstSrcRegs.dstSrcMap.end();
       ) {
    std::vector<regMapBRA>::iterator nextIter = dstSrcRegsIter;
    nextIter++;
    bool erased = false;
    for (const auto& notdstSrcReg : notRemoveableMap) {
      if ((*dstSrcRegsIter).opndNum == notdstSrcReg.first &&
          (*dstSrcRegsIter).offset == notdstSrcReg.second) {
        dstSrcRegsIter = dstSrcRegs.dstSrcMap.erase(dstSrcRegsIter);
        movInstNum--;
        erased = true;
        break;
      }
    }
    if (!erased) {
      dstSrcRegsIter = nextIter;
    }
  }

  // Check if there are enough mov instructions to be removed.
  if (movInstNum < 2) {
    return false;
  }

  dstSrcRegs.firstDefID = firstDefID;
  // Sort according to the register order in the original payload
  std::sort(dstSrcRegs.dstSrcMap.begin(), dstSrcRegs.dstSrcMap.end(),
            regSortCompareAfterRA);

  return true;
}


// Replace the send instruction with the payload of
// Insert the scalar register intialization mov instructions.
bool SRSubPassAfterRA::replaceWithSendiAfterRA(G4_BB *bb,
                                               INST_LIST_ITER instIter,
                                               regCandidatesBRA &dstSrcRegs) {
  G4_INST *inst = *instIter;
  std::vector<G4_AddrExp *> srcs;
  G4_AddrExp *src;

  srcs.resize(MAXIMAL_S0_SRC0_GRF_LENGTH);
  for (int i = 0; i < MAXIMAL_S0_SRC0_GRF_LENGTH; i++) {
    srcs[i] = nullptr;
  }

  unsigned short GRFSize = builder.getGRFSize();
  int totalRegs = 0;
  int j = 0;

  //Add source of define mov if the mov can be removed, otherwise, add the source 0 directly
  int src0Size = inst->getMsgDesc()->getSrc0LenRegs();
  G4_Operand *src0 = inst->getSrc(0);
  for (int i = 0; i < src0Size; i++) {
    bool replaced = false;
    if (j < (int)dstSrcRegs.dstSrcMap.size() &&
        dstSrcRegs.dstSrcMap[j].opndNum == Opnd_src0) {
      int opndSize =
          (dstSrcRegs.dstSrcMap[j].opnd->getLinearizedEnd() -
           dstSrcRegs.dstSrcMap[j].opnd->getLinearizedStart() + GRFSize - 1) /
          GRFSize;
      int srcOffset = src0->getLeftBound() / GRFSize + i;
      int opndOffset = dstSrcRegs.dstSrcMap[j].offset;

      if ((srcOffset >= opndOffset) && (srcOffset < opndOffset + opndSize)) {
        for (int k = 0; k < opndSize; k++) {
          src = builder.createAddrExp(
              dstSrcRegs.dstSrcMap[j].opnd->getTopDcl()->getRegVar(),
              dstSrcRegs.dstSrcMap[j].opnd->getLeftBound() + k * GRFSize,
              Type_UW);
          srcs[totalRegs] = src;
          dstSrcRegs.dstSrcMap[j].opnd->getTopDcl()->setDoNotSpill();
          dstSrcRegs.dstSrcMap[j].opnd->getTopDcl()->setAddressed();
          totalRegs++;
        }
        replaced = true;
        i += opndSize - 1;
        j++;
      }
    }

    if (!replaced) {
      // Add source 0 register directly
      src = builder.createAddrExp(src0->getTopDcl()->getRegVar(),
                                  src0->getLeftBound() + i * GRFSize, Type_UW);
      srcs[totalRegs] = src;
      src0->getTopDcl()->setDoNotSpill();
      src0->getTopDcl()->setAddressed();
      totalRegs++;
    }
  }

  // Add source of define mov if the mov can be removed, otherwise, add the
  // source 1 directly
  G4_Operand *src1 = inst->getSrc(1);
  if (src1 && !src1->isNullReg()) {
    int src1Size = inst->getMsgDesc()->getSrc1LenRegs();
    for (int i = 0; i < src1Size; i++) {
      bool replaced = false;
      if (j < (int)dstSrcRegs.dstSrcMap.size() &&
          dstSrcRegs.dstSrcMap[j].opndNum == Opnd_src1) {
        int opndSize =
            (dstSrcRegs.dstSrcMap[j].opnd->getLinearizedEnd() -
             dstSrcRegs.dstSrcMap[j].opnd->getLinearizedStart() + GRFSize - 1) /
            GRFSize;
        int srcOffset = src1->getLeftBound() / GRFSize + i;
        int opndOffset = dstSrcRegs.dstSrcMap[j].offset;

        if ((srcOffset >= opndOffset) && (srcOffset < opndOffset + opndSize)) {
          for (int k = 0; k < opndSize; k++) {
            src = builder.createAddrExp(
                dstSrcRegs.dstSrcMap[j].opnd->getTopDcl()->getRegVar(),
                dstSrcRegs.dstSrcMap[j].opnd->getLeftBound() + k * GRFSize,
                Type_UW);
            dstSrcRegs.dstSrcMap[j].opnd->getTopDcl()->setDoNotSpill();
            dstSrcRegs.dstSrcMap[j].opnd->getTopDcl()->setAddressed();
            srcs[totalRegs] = src;
            totalRegs++;
          }
          replaced = true;
          i += opndSize - 1;
          j++;
        }
      }

      if (!replaced) {
        // Add source 1 register directly
        src =
            builder.createAddrExp(src1->getTopDcl()->getRegVar(),
                                  src1->getLeftBound() + i * GRFSize, Type_UW);
        srcs[totalRegs] = src;
        src1->getTopDcl()->setDoNotSpill();
        src1->getTopDcl()->setAddressed();
        totalRegs++;
      }
    }
  }

  // Initialize the scalar registers.
  uint16_t UQNum = totalRegs > (TypeSize(Type_UQ) / TypeSize(Type_UB)) ? 2 : 1;
  if (dstSrcRegs.isLargeGRF) {
    UQNum = totalRegs > (TypeSize(Type_UQ) / TypeSize(Type_UW)) ? 2 : 1;
  }
  G4_Declare *s0Var = builder.createTempScalar(UQNum, "S0_");
  s0Var->getRegVar()->setPhyReg(builder.phyregpool.getScalarReg(), 0);
  G4_DstRegRegion *dst =
      builder.createDst(s0Var->getRegVar(), 0, 0, 1, Type_UQ);
  G4_INST *movInst = nullptr;
  if (!dstSrcRegs.isLargeGRF) {
    movInst = builder.createIntrinsicAddrMovInst(
        Intrinsic::PseudoAddrMov, dst, srcs[0], srcs[1], srcs[2], srcs[3],
        srcs[4], srcs[5], srcs[6], srcs[7], false);
  } else {
    movInst = builder.createIntrinsicAddrMovInst(
        Intrinsic::PseudoAddrMovW, dst, srcs[0], srcs[1], srcs[2], srcs[3],
        nullptr, nullptr, nullptr, nullptr, false);
  }
  bb->insertBefore(instIter, movInst);

  if (UQNum > 1) {
    G4_DstRegRegion *dst1 =
        builder.createDst(s0Var->getRegVar(), 0, 1, 1, Type_UQ);
    G4_INST *movInst1 = nullptr;
    if (!dstSrcRegs.isLargeGRF) {
      movInst1 = builder.createIntrinsicAddrMovInst(
          Intrinsic::PseudoAddrMov, dst1, srcs[8], srcs[9], srcs[10], srcs[11],
          srcs[12], srcs[13], srcs[14], nullptr, false);
    } else {
      movInst1 = builder.createIntrinsicAddrMovInst(
          Intrinsic::PseudoAddrMovW, dst1, srcs[4], srcs[5], srcs[6], srcs[7],
          nullptr, nullptr, nullptr, nullptr, false);
    }
    bb->insertBefore(instIter, movInst1);
  }

  auto gtpinData = kernel.getGTPinData();
  if (gtpinData) {
    for (auto addrTaken : srcs) {
      if (!addrTaken)
        continue;
      auto *targetDcl =
          addrTaken->asAddrExp()->getRegVar()->getDeclare()->getRootDeclare();
      gtpinData->addIndirRef(s0Var, targetDcl);
    }
  }

  changeToIndirectSend(inst, s0Var, totalRegs, builder, dstSrcRegs.isLargeGRF);

  return true;
}

void SRSubPassAfterRA::SRSubAfterRA(G4_BB *bb) {
  bb->resetLocalIds();

  class CmpFirstDef {
  public:
    bool operator()(G4_INST *first, G4_INST *second) const {
      return first->getLocalId() < second->getLocalId();
    }
  };
  std::map<G4_INST *, regCandidatesBRA> candidates;
  std::map<G4_INST *, regCandidatesBRA>::iterator candidatesIt;

  INST_LIST_ITER ii = bb->begin(), iend(bb->end());
  unsigned candidateStart = builder.getuint32Option(vISA_IndirectInstStart);
  unsigned candidateEnd = builder.getuint32Option(vISA_IndirectInstEnd);
  while (ii != iend) {
    G4_INST *inst = *ii;

    regCandidatesBRA dstSrcRegs;
    if (!isSRCandidateAfterRA(inst, dstSrcRegs)) {
      ii++;
      dstSrcRegs.dstSrcMap.clear();
      continue;
    }
    candidateID++;
    // if candidateStart is 0, always do it.
    if ((candidateStart != 0) && (candidateStart <= candidateEnd) &&
        ((candidateID < candidateStart) ||
         (candidateID > candidateEnd))) { // Don't do it when it's out of range
      ii++;
      dstSrcRegs.dstSrcMap.clear();
      continue;
    }

    candidates.insert(std::make_pair(inst, dstSrcRegs));
    ii++;
  }

  // If the registser is redefined before the send, the substitution needs be
  // cancelled. Such as r9 in following instructions, which is redefined in sel
  // instruction.
  // mov(8)             r6.0< 1 > :f  r9.0 < 1; 1, 0 > : f
  // sel(8) (ge)f0.0    r9.0< 1 > :f  r57.0 < 1; 1, 0 > : f
  // mov(8)             r3.0< 1 > :f  r79.0 < 1; 1, 0 > : f
  // mov(8)             r4.0 < 1 > : f  r27.0 < 1; 1, 0 > : f
  // mov(8)             r5.0 < 1 > : f  r77.0 < 1; 1, 0 > : f
  // mov(8)             r2.0 < 1 > : d  r111.0 < 1; 1, 0 > : d
  // sends(8)           null : ud r112 r2 0x14c : ud 0x2035092 : ud //typed
  // surface write, resLen=0, msgLen=1, extMsgLen=5
  INST_LIST_RITER ri = bb->rbegin(), rend(bb->rend());
  // Scan backward
  while (ri != rend) {
    G4_INST *inst = *ri;
    candidatesIt = candidates.find(inst);
    //Is candidate send
    if (candidatesIt != candidates.end()) {
      // Scan backward from the send instruction.
      INST_LIST_RITER scan_ri = ri;
      scan_ri++;
      G4_INST *rInst = *scan_ri;

      while (rInst->getLocalId() > candidates[inst].firstDefID) {
        if (rInst->isDead()) {
          // If the inst is marked as dead, it's dst will not kill other value
          //  Such as in following case, if third instruction is removed, r64
          //  value of first instruction is kept.
          // mov (16)             r16.0<1>:ud  r64.0<1;1,0>:ud // $214:&226:
          // mov (16)             r17.0<1>:ud  r66.0<1;1,0>:ud // $216:&228:
          // mov (16)             r64.0<1>:ud  r68.0<1;1,0>:ud // $218:&230:
          scan_ri++;
          if (scan_ri == rend) {
            break;
          }
          rInst = *scan_ri;
          continue;
        }
        G4_Operand *dst = rInst->getDst();
        if (dst && !dst->isNullReg()) {
          G4_VarBase *base = dst->getBase();
          G4_VarBase *phyReg =
              (base->isRegVar()) ? base->asRegVar()->getPhyReg() : base;

          if (phyReg->getKind() == G4_VarBase::VK_phyGReg) {
            // If the register redefined happened in the mov instruction which will
            // be removed. Or the instruction whose dst will be accessed through
            // s0. It's not real reuse. Such as the r8 in following
            // instructions.
            // clang-format off
            // mov(8)  r9.0 <1>:f  r8.0<1; 1, 0>:f
            // mov(8)  r10.0<1>:f  r3.0<8; 8, 1>:f
            // mov(8)  r11.0<1>:f  r3.0<8; 8, 1>:f
            // mov(8)  r7.0 <1>:d  r5.0<1; 1, 0>:d
            // mov(8)  r8.0 <1>:f  r10.0<1; 1, 0>:f //assume cannot be removed
            // sends(8) null:ud r112 r7 0x14c : ud 0x2035093 : ud // typed
            // surface write, resLen=0, msgLen=1, extMsgLen=5
            // clang-format on
            G4_Operand *dst = rInst->getDst();
            unsigned short dstRegLB = dst->getLinearizedStart();
            unsigned short dstRegRB = dst->getLinearizedEnd();

            // There is any none removeable offset, the offset define move
            // cannot be removed.
            std::vector<regMapBRA>::iterator dstSrcRegsIter;
            for (dstSrcRegsIter = candidates[inst].dstSrcMap.begin();
                 dstSrcRegsIter != candidates[inst].dstSrcMap.end();) {
              std::vector<regMapBRA>::iterator nextIter = dstSrcRegsIter;
              nextIter++;

              // If reverse scan instruction local ID is equal or less than removed
              // instruction local ID, there is no need to check.
              if (rInst->getLocalId() <= (*dstSrcRegsIter).inst->getLocalId()) {
                dstSrcRegsIter = nextIter;
                continue;
              }
              int srcRegLB = (*dstSrcRegsIter).opnd->getLinearizedStart();
              int srcRegRB = (*dstSrcRegsIter).opnd->getLinearizedEnd();
              if (!(srcRegRB < dstRegLB || srcRegLB > dstRegRB)) {

                // Register is redefined
                dstSrcRegsIter =
                    candidates[inst].dstSrcMap.erase(dstSrcRegsIter);
              } else {
                dstSrcRegsIter = nextIter;
              }
            }
          }
        }
        scan_ri++;
        if (scan_ri == rend) {
          break;
        }
        rInst = *scan_ri;
      }

      // Due to extra mov for s0, so don't use s0 if equal or less than 1 mov
      // inst can be removed.
      if (candidates[inst].dstSrcMap.size() <= 1 &&
          builder.getuint32Option(vISA_EnableGatherWithImmPreRA) !=
              INDIRECT_TYPE::ALWAYS_S0) {
        candidates.erase(candidatesIt);
      } else {
        for (int j = 0; j < (int)candidatesIt->second.dstSrcMap.size(); j++) {
          G4_INST *movInst = candidatesIt->second.dstSrcMap[j].inst;
          movInst->markDead();
        }
      }
    }

    ri++;
  }

  // Replace the send instruction with sendi
  // Remove the mov instructions that marked as dead
  INST_LIST_ITER iter;
  for (iter = bb->begin(); iter != bb->end();) {
    G4_INST *inst = *iter;
    INST_LIST_ITER curIter = iter;
    iter++;

    candidatesIt = candidates.find(inst);
    if (candidatesIt != candidates.end()) {
      replaceWithSendiAfterRA(bb, curIter, candidates[inst]);
    }
    if (inst->isDead()) {
      bb->erase(curIter);
    }
  }
}
