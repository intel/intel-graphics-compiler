/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SpillCode.h"
#include "BuildIR.h"
#include "FlowGraph.h"
#include "PointsToAnalysis.h"
#include <vector>

#ifdef _DEBUG
#define _DEBUG_SPILL
#endif

using namespace vISA;

void splice(G4_BB *bb, INST_LIST_ITER iter, INST_LIST &instList,
            unsigned int CISAOff);

//
// create a declare to hold the spill value of var
//
G4_Declare *SpillManager::createNewSpillLocDeclare(G4_Declare *dcl) {

  if (dcl->getRegFile() == G4_FLAG) {
    vISA_ASSERT(dcl->getElemType() == Type_UW || dcl->getElemType() == Type_W,
                "flag reg's type should be UW");
    vISA_ASSERT(dcl->getNumElems() <= builder.getNumFlagRegisters(),
                "Flag reg Spill size exceeds limit");
  } else if (dcl->getRegFile() == G4_ADDRESS) {
    // if we are dealing with type other than UW, e.g., B, then we need to
    // take care different data type reg moves of spill code. For now, just
    // assume data types of addr reg are UW
    //
    [[maybe_unused]] G4_Type type = dcl->getElemType();
    vISA_ASSERT(type == Type_UW || type == Type_W || type == Type_UD ||
                    type == Type_D,
                "addr reg's type should be UW or UD");
    vISA_ASSERT(dcl->getNumElems() <= builder.getNumAddrRegisters(),
                "Addr reg Spill size exceeds limit");
  }
  G4_Declare *sp = dcl->getSpilledDeclare();
  if (sp == NULL) // not yet created
  {
    sp = builder.createAddrFlagSpillLoc(dcl);
    gra.setBBId(sp, bbId);
  }

  return sp;
}

//
// replicate dcl for temporary use (loading value from SPILL location)
//
G4_Declare *SpillManager::createNewTempAddrDeclare(G4_Declare *dcl) {
  const char *name = builder.getNameString(16, "Temp_ADDR_%d", tempDclId++);

  vISA_ASSERT(dcl->getElemType() == Type_UW || dcl->getElemType() == Type_W,
              "addr reg's type should be UW");
  vISA_ASSERT(dcl->getNumRows() == 1, "Temp_ADDR should be only 1 row");
  vISA_ASSERT(dcl->getNumElems() <= builder.getNumAddrRegisters(),
              "Temp_ADDR element number exceeds maximum value");
  G4_Declare *sp = builder.createDeclare(name, G4_ADDRESS, dcl->getNumElems(),
                                         1, // 1 row
                                         Type_UW);

  // When creating temp address variable, same sub align is needed
  sp->setSubRegAlign(dcl->getSubRegAlign());
  gra.setBBId(sp, bbId);
  // Live range of new temp addrs is short so that there is no point spilling
  // them. indicate this is for newly created addr temp so that RA won't spill
  // it in subsequent RA allocation
  gra.addAddrFlagSpillDcl(sp);

  return sp;
}

G4_Declare *SpillManager::createNewTempFlagDeclare(G4_Declare *dcl) {
  const char *name = builder.getNameString(32, "Temp_FSPILL_%d", tempDclId++);

  vISA_ASSERT(dcl->getRegFile() == G4_FLAG, "dcl should be a flag");
  G4_Declare *sp = builder.createFlag(dcl->getNumberFlagElements(), name);
  gra.setBBId(sp, bbId);
  sp->copyAlign(dcl);
  gra.copyAlignment(sp, dcl);
  gra.addAddrFlagSpillDcl(sp);

  return sp;
}

//
// replicate dcl for temporary use (loading value from SPILL location)
//
G4_Declare *SpillManager::createNewTempAddrDeclare(G4_Declare *dcl,
                                                   uint16_t num_reg) {
  const char *name = builder.getNameString(16, "Temp_ADDR_%d", tempDclId++);

  G4_Type type = dcl->getElemType();
  vISA_ASSERT(type == Type_UW || type == Type_W || type == Type_UD ||
                  type == Type_D,
              "addr reg's type should be UW or UD");
  vISA_ASSERT(dcl->getNumRows() == 1, "Temp_ADDR should be only 1 row");
  vISA_ASSERT(dcl->getNumElems() <= builder.getNumAddrRegisters(),
              "Temp_ADDR exceeds 16 bytes");
  G4_Declare *sp = builder.createDeclare(name, G4_ADDRESS, num_reg,
                                         1, // 1 row
                                         type);
  //The address register is in size of W
  auto subAlign = Get_G4_SubRegAlign_From_Size(
      num_reg * 2, builder.getPlatform(), builder.getGRFAlign());

  sp->setSubRegAlign(subAlign);
  gra.setBBId(sp, bbId);
  // Live range of new temp addrs is short so that there is no point spilling
  // them. indicate this is for newly created addr temp so that RA won't spill
  // it in subsequent RA allocation
  gra.addAddrFlagSpillDcl(sp);

  return sp;
}

G4_Declare *SpillManager::createNewTempScalarDeclare(G4_Declare *dcl) {
  const char *name = builder.getNameString(32, "Temp_SCALAR_%d", tempDclId++);
  G4_Declare *sp =
      builder.createDeclare(name, G4_SCALAR, 1, 1, dcl->getElemType());
  gra.setBBId(sp, bbId);
  sp->copyAlign(dcl);
  auto subAlign = Get_G4_SubRegAlign_From_Size((uint16_t)dcl->getElemSize(),
                                               builder.getPlatform(),
                                               builder.getGRFAlign());
  sp->setSubRegAlign(subAlign);
  gra.addAddrFlagSpillDcl(sp);

  return sp;
}

//
// generate a reg to reg mov inst for addr/flag spill
// mov  dst(dRegOff,dSubRegOff)<1>  src(sRegOff,sSubRegOff)<nRegs;nRegs,1>
//
void SpillManager::genRegMov(G4_BB *bb, INST_LIST_ITER it, G4_VarBase *src,
                             unsigned short sSubRegOff, G4_VarBase *dst,
                             unsigned nRegs, bool useNoMask = true) {
  builder.instList.clear();

  uint16_t dSubRegOff = 0;
  for (uint16_t i = builder.getNumAddrRegisters(); i != 0 && nRegs != 0;
       i >>= 1) // 16, 8, 4, 2, 1
  {
    if (nRegs >= i) {
      //
      // create loc(0,locOff)<nRegs;nRegs,1> operand
      //
      /*
          Flag registers should always be scalar regions
      */
      G4_Type type = Type_W;
      const RegionDesc *srcRgn = NULL;
      G4_ExecSize execSize{i};
      if (src->isFlag() || dst->isFlag()) {

        type = Type_UW;
        if (i == 2) {
          type = Type_UD;
          execSize = g4::SIMD1;
        } else if (i > 2) {
          vISA_ASSERT(false, "unsupported flag width");
        }

        srcRgn = builder.getRegionScalar();
      } else {
        srcRgn =
            (i == 1) ? builder.getRegionScalar() : builder.getRegionStride1();
      }

      G4_SrcRegRegion *s = builder.createSrc(src, 0, sSubRegOff, srcRgn, type);
      //
      // create a0.aOff<1>
      //
      G4_DstRegRegion *d = builder.createDst(dst, 0, dSubRegOff, 1, type);

      if (execSize != kernel.getSimdSize()) {
        // NoMask must be used in this case
        useNoMask = true;
      }
      // mov (nRegs)  a0.aOff<1>  loc(0,locOff)<4;4,1>
      builder.createMov(execSize, d, s,
                        useNoMask ? InstOpt_WriteEnable : InstOpt_NoOpt, true);

      sSubRegOff += i;
      dSubRegOff += i;

      nRegs -= i;
    }
  }
  vISA_ASSERT(nRegs == 0, ERROR_SPILLCODE);

  if (gra.EUFusionNoMaskWANeeded()) {
    for (auto inst : builder.instList) {
      if (inst->isWriteEnableInst()) {
        gra.addEUFusionNoMaskWAInst(bb, inst);
      }
    }
  }

  //
  // insert newly created insts from builder to instList
  //
  splice(bb, it, builder.instList, currCISAOffset);
}
//
// check if dst is spilled & insert spill code
//
void SpillManager::replaceSpilledDst(
    G4_BB *bb,
    INST_LIST_ITER it, // where new insts will be inserted
    G4_INST *inst, std::vector<G4_Operand *> &operands_analyzed,
    std::vector<G4_Declare *> &declares_created) {
  G4_DstRegRegion *dst = inst->getDst();
  if (dst == NULL)
    return;

  if (dst->getBase()->isRegAllocPartaker() &&
      dst->getBase()->asRegVar()->getDeclare()->getSpilledDeclare() !=
          NULL) // spilled base
  {
    // create a dst region with spill loc
    // original dst region  V100_uw(0,0)<1>:uw ==>
    // new dst region SP_uw(0,0)<1>:uw
    G4_Declare *spDcl =
        dst->getBase()->asRegVar()->getDeclare()->getSpilledDeclare();
    if (dst->getRegAccess() == Direct) {

      G4_DstRegRegion rgn(builder, *dst,
                          spDcl->getRegVar()); // using spDcl as new base
      if (rgn.getHorzStride() == UNDEFINED_SHORT && dst->isFlag()) {
        // Flag as destination has undefined hstride
        // For replacing it with spill range, make hstride 1
        rgn.setHorzStride(1);
      }
      G4_DstRegRegion *d = builder.createDstRegRegion(rgn);
      inst->setDest(d);
    } else if (dst->getRegAccess() == IndirGRF) {
      // add (1)  r[V100_uw(0,0),0]<1>:f V124_f(0,0)<0;1,0>:f  1
      // indirect access' base must be addr reg so we need to create a temp addr
      // live range to load value from V100's spill loc e.g.,   mov (1)
      // T_uw(0,0)<1>:uw SPILL_LOC_V100_uw(0,0)<0;1,0>:uw
      //         add (1)  r[T_uw(0,0),0]<1>:f V124_f(0,0)<0;1,0>:f   1
      //
      // create declare for temp addr live range
      //
      G4_Declare *tmpDcl = NULL;
      bool match_found = false;

      for (unsigned int j = 0, je = operands_analyzed.size(); j < je; j++) {
        G4_SrcRegRegion *analyzed_src =
            static_cast<G4_SrcRegRegion *>(operands_analyzed[j]);
        if (analyzed_src != NULL &&
            analyzed_src->getBase()->asRegVar()->getDeclare() ==
                dst->getBase()->asRegVar()->getDeclare() &&
            analyzed_src->getSubRegOff() == dst->getSubRegOff() &&
            !analyzed_src->getRegion()->isRegionWH()) {
          tmpDcl = declares_created[j];
          match_found = true;
        }
      }

      if (!match_found) {
        tmpDcl = createNewTempAddrDeclare(spDcl);
        //
        // generate mov Tmp(0,0)<1>  SPILL_LOC_V100(0,0)
        //
        genRegMov(bb, it, spDcl->getRegVar(), 0, tmpDcl->getRegVar(),
                  tmpDcl->getNumElems());
      }

      G4_DstRegRegion rgn(builder, *dst,
                          tmpDcl->getRegVar()); // using tmpDcl as new base
      G4_DstRegRegion *d = match_found
                               ? builder.createDstWithNewSubRegOff(&rgn, 0)
                               : builder.createDstRegRegion(rgn);
      inst->setDest(d);

      if (!match_found) {
        pointsToAnalysis.insertAndMergeFilledAddr(dst->getBase()->asRegVar(),
                                                  tmpDcl->getRegVar());
      }
    } else
      vISA_ASSERT(false, "Unknown reg access");
  }
}
//
// check if src is spilled and insert spill code to load spilled value
//
void SpillManager::replaceSpilledSrc(
    G4_BB *bb,
    INST_LIST_ITER it, // where new insts will be inserted
    G4_INST *inst, unsigned i, std::vector<G4_Operand *> &operands_analyzed,
    std::vector<G4_Declare *> &declares_created) {
  G4_Operand *src = inst->getSrc(i);
  if (src == NULL)
    return;
  //
  // go ahead replace src (addr reg) with spill GRF
  //

  if (src->isSrcRegRegion() &&
      src->asSrcRegRegion()->getBase()->isRegAllocPartaker() &&
      src->asSrcRegRegion()
              ->getBase()
              ->asRegVar()
              ->getDeclare()
              ->getSpilledDeclare() != NULL) // spilled base
  {
    // create a src region with spill loc
    // original src region  V100_uw(0,0)<0;1,0>:uw
    // new src region SP_uw(0,0)<0;1,0>:uw
    G4_SrcRegRegion *ss = src->asSrcRegRegion();
    G4_Declare *srcDcl = ss->getBase()->asRegVar()->getDeclare();
    G4_Declare *spDcl = srcDcl->getSpilledDeclare();
    if (ss->getRegAccess() == Direct) {
      G4_SrcRegRegion *s;
      if (inst->isSplitSend() && i == 3) {
        G4_Declare *tmpDcl = createNewTempAddrDeclare(spDcl, 1);
        tmpDcl->setSubRegAlign(Four_Word);
        gra.setSubRegAlign(tmpDcl, Four_Word);
        // (W) mov (1) tmpDcl<1>:ud spDcl<0;1,0>:ud
        auto movSrc =
            builder.createSrcRegRegion(spDcl, builder.getRegionScalar());
        auto movDst = builder.createDstRegRegion(tmpDcl, 1);
        G4_INST *movInst = builder.createMov(g4::SIMD1, movDst, movSrc,
                                             InstOpt_WriteEnable, false);
        bb->insertBefore(it, movInst);

        if (gra.EUFusionNoMaskWANeeded()) {
          gra.addEUFusionNoMaskWAInst(bb, movInst);
        }

        s = builder.createSrc(tmpDcl->getRegVar(), 0, 0, ss->getRegion(),
                              spDcl->getElemType());
        inst->setSrc(s, i);
      } else {
        s = builder.createSrcWithNewBase(
            ss, spDcl->getRegVar()); // using spDcl as new base
      }
      inst->setSrc(s, i);
    } else if (ss->getRegAccess() == IndirGRF) {
      // add (2)  V124_f(0,0)<1>:f  r[V100_uw(0,0),0]<4;2,2>:f 1
      // indirect access' base must be addr reg so we need to create a temp addr
      // live range to load value from V100's spill loc e.g.,   mov (1)
      // T(0,0)<1>:uw SPILL_LOC_V100_uw(0,0)<0;1,0>:uw
      //         add (2)  V124_f(0,0)<1>:f  r[T(0,0),0]<4;2,2>:f 1
      //
      // create declare for temp addr live range
      //

      uint16_t num_reg = 1;
      // if access is VxH copy number of addresses based on execution size of
      // instruction
      if (ss->getRegion()->isRegionWH()) {
        num_reg = inst->getExecSize();
      }

      G4_Declare *tmpDcl = NULL;
      bool match_found = false;

      for (unsigned int j = 0; j < i; j++) {
        G4_SrcRegRegion *analyzed_src = (G4_SrcRegRegion *)operands_analyzed[j];
        if (analyzed_src != NULL &&
            analyzed_src->getBase()->asRegVar()->getDeclare() ==
                ss->getBase()->asRegVar()->getDeclare() &&
            analyzed_src->getSubRegOff() == ss->getSubRegOff()) {
          tmpDcl = declares_created[j];
          match_found = true;
        }
      }

      if (!match_found) {
        tmpDcl = createNewTempAddrDeclare(spDcl, num_reg);
        operands_analyzed[i] = ss;
        declares_created[i] = tmpDcl;
        bool isNoMask = (inst->getOption() & InstOpt_WriteEnable) != 0;
        //
        // generate mov Tmp(0,0)<1>  SPILL_LOC_V100(0,0)
        //
        genRegMov(bb, it, spDcl->getRegVar(), ss->getSubRegOff(),
                  tmpDcl->getRegVar(), tmpDcl->getNumElems(),
                  isNoMask);
        auto pseudoKill =
          builder.createPseudoKill(tmpDcl, PseudoKillType::Other, false);
        bb->insertBefore(std::prev(it), pseudoKill);
      }

      // create new src from the temp address variable, with offset 0
      auto s = builder.createIndirectSrc(ss->getModifier(), tmpDcl->getRegVar(),
                                         ss->getRegOff(), 0, ss->getRegion(),
                                         ss->getType(), ss->getAddrImm());
      inst->setSrc(s, i);
      if (!match_found) {
        pointsToAnalysis.insertAndMergeFilledAddr(ss->getBase()->asRegVar(),
                                                  tmpDcl->getRegVar());
      }
    } else
      vISA_ASSERT(false, "Unknown reg access");
  }
}

//
// check if predicate is spilled & insert fill code
//
void SpillManager::replaceSpilledPredicate(
    G4_BB *bb,
    INST_LIST_ITER it, // where new insts will be inserted
    G4_INST *inst) {
  G4_Predicate *predicate = inst->getPredicate();
  if (predicate == NULL)
    return;

  G4_VarBase *flagReg = predicate->getBase();
  if (flagReg->asRegVar()->isRegAllocPartaker()) {
    G4_Declare *flagDcl = flagReg->asRegVar()->getDeclare();
    G4_Declare *spDcl = flagDcl->getSpilledDeclare();
    if (spDcl != NULL) {
      G4_Declare *tmpDcl = createNewTempFlagDeclare(flagDcl);
      genRegMov(bb, it, spDcl->getRegVar(), 0, tmpDcl->getRegVar(),
                tmpDcl->getNumElems());
      G4_Predicate *new_pred =
          builder.createPredicate(predicate->getState(), tmpDcl->getRegVar(), 0,
                                  predicate->getControl());
      inst->setPredicate(new_pred);
      ++numFlagSpillLoad;
    }
  }
}

bool SpillManager::checkDefUseDomRel(G4_Operand *dst, G4_BB *defBB) {
  auto dcl = dst->getTopDcl();

  // check whether this def dominates all its uses
  auto uses = refs.getUses(dcl);

  for (auto &use : *uses) {
    auto useBB = std::get<1>(use);

    // check if def dominates use
    if (!defBB->dominates(useBB))
      return false;

    if (defBB == useBB) {
      // defBB dominates useBB since its the same BB.
      // ensure def instruction appears lexically before use BB.
      auto useInst = std::get<0>(use);
      if (dst->getInst()->getLexicalId() > useInst->getLexicalId())
        return false;
    }
  }

  // if def is in loop then ensure all uses are in same loop level
  // or inner loop nest of def's closest loop.
  auto defLoop = gra.kernel.fg.getLoops().getInnerMostLoop(defBB);
  if (defLoop) {
    // since def is in loop, check whether uses are also in same loop.
    for (auto &use : *uses) {
      auto useBB = std::get<1>(use);
      auto useLoop = gra.kernel.fg.getLoops().getInnerMostLoop(useBB);
      if (!useLoop)
        return false;

      if (!useLoop->fullSubset(defLoop))
        return false;
    }
  }

  return true;
}

bool SpillManager::isDominatingDef(G4_Operand *opnd, G4_BB *bb) {
  // return true if this opnd is dominates all other defs
  auto dcl = opnd->getTopDcl();

  auto defs = refs.getDefs(dcl);

  for (auto &def : *defs) {
    auto otherDefBB = std::get<1>(def);

    if (!bb->dominates(otherDefBB))
      return false;

    if (bb == otherDefBB) {
      auto otherDefInst = std::get<0>(def);
      if (opnd->getInst()->getLexicalId() > otherDefInst->getLexicalId())
        return false;
    }
  }

  return true;
}

// update RMW information for flag operands
void SpillManager::updateRMWNeeded() {
  if (!gra.kernel.getOption(vISA_SkipRedundantFillInRMW))
    return;

  auto isRMWNeededForSpill = [&](G4_BB *bb, G4_Operand *spilledRegion) {
    bool isUniqueDef = (refs.getDefCount(spilledRegion->getTopDcl()) < 2);

    // Check0 : Def is NoMask, -- checked before inserting RMW fill already
    // Check1 : Def is unique def OR def is dominating all other defs,
    // Check2 : Def is in loop L and all use(s) of dcl are in loop L or it's
    // inner loop nest Check3 : Flowgraph is reducible RMW_Not_Needed = Check0
    // || (Check1 && Check2 && Check3)
    bool RMW_Needed = true;

    if ((isUniqueDef || isDominatingDef(spilledRegion, bb)) &&
        kernel.fg.isReducible() && checkDefUseDomRel(spilledRegion, bb)) {
      RMW_Needed = false;
    }

    return RMW_Needed;
  };

  auto isOpndSpilled = [&](G4_Operand *opnd) {
    auto base = opnd->getBase();
    if (base && base->isRegVar() && base->asRegVar()->isRegAllocPartaker()) {
      auto dstRegVar = base->asRegVar();
      if (dstRegVar && base->asRegVar()->getDeclare()->getSpilledDeclare())
        return true;
    }
    return false;
  };

  // update rmw set if opnd is spilled, nop if opnd isnt spilled.
  auto updateRMW = [&](G4_BB *bb, G4_Operand *opnd) {
    auto RMW_Needed = isRMWNeededForSpill(bb, opnd);
    if (!RMW_Needed) {
      // Any spilled dst region that doesnt need RMW
      // is added to noRMWNeeded set. This set is later
      // checked when inserting spill/fill code.
      noRMWNeeded.insert(opnd);
    }
  };

  // First pass to setup lexical ids of instruction so dominator relation can be
  // computed correctly intra-BB.
  unsigned int lexId = 0;
  for (auto bb : gra.kernel.fg.getBBList()) {
    for (auto inst : bb->getInstList()) {
      inst->setLexicalId(lexId++);
    }
  }

  for (auto bb : gra.kernel.fg.getBBList()) {
    for (auto inst : bb->getInstList()) {
      if (inst->isPseudoKill())
        continue;

      // flags are used only as cond modifiers with non-NoMask
      auto condMod = inst->getCondMod();
      if (condMod && isOpndSpilled(condMod)) {
        updateRMW(bb, condMod);
      }
    }
  }
}

//
// check if flag dst is spilled and insert spill code
//
void SpillManager::replaceSpilledFlagCondMod(
    G4_BB *bb,
    INST_LIST_ITER it, // where new insts will be inserted
    G4_INST *inst) {
  G4_CondMod *mod = inst->getCondMod();
  if (mod == NULL)
    return;

  G4_VarBase *flagReg = mod->getBase();
  if (flagReg != NULL && flagReg->asRegVar()->isRegAllocPartaker()) {
    G4_Declare *flagDcl = flagReg->asRegVar()->getDeclare();
    G4_Declare *spDcl = flagDcl->getSpilledDeclare();
    if (spDcl != NULL) {
      G4_Declare *tmpDcl;
      G4_Predicate *predicate = inst->getPredicate();

      if (predicate != NULL) {
        G4_VarBase *flagReg = predicate->getBase();
        tmpDcl = flagReg->asRegVar()->getDeclare();
      } else {
        tmpDcl = createNewTempFlagDeclare(flagDcl);
      }

      // Need to pre-load the spill GRF if the inst isn't going to write the
      // full spilled GRF variable.
      if (flagDcl->getNumberFlagElements() > inst->getExecSize() ||
          (!bb->isAllLaneActive() && !inst->isWriteEnableInst())) {
        // Conditional modifier must use same flag register as predicate.
        // So if conditional modifier needs pre-fill, the prediciate must be
        // filled already.
        if (noRMWNeeded.find(mod) == noRMWNeeded.end() && predicate == NULL) {
          genRegMov(bb, it, spDcl->getRegVar(), 0, tmpDcl->getRegVar(),
                    tmpDcl->getNumElems());
          ++numFlagSpillLoad;
        } else {
          // insert kill for temp flag
          auto pseudoKill =
              builder.createPseudoKill(tmpDcl, PseudoKillType::Other, false);
          bb->insertBefore(it, pseudoKill);
        }
      }

      G4_CondMod *newCondMod =
          builder.createCondMod(mod->getMod(), tmpDcl->getRegVar(), 0);

      inst->setCondMod(newCondMod);

      genRegMov(bb, ++it, tmpDcl->getRegVar(), 0, spDcl->getRegVar(),
                tmpDcl->getNumElems());
      ++numFlagSpillStore;
    }
  }
}

//
// go over all declares and allocate spill locations
//
void SpillManager::createSpillLocations(const G4_Kernel &kernel) {
  // set spill flag to indicate which vars are spilled
  for (const LiveRange *lr : spilledLRs) {
    G4_Declare *dcl = lr->getVar()->getDeclare();
    dcl->setSpillFlag();
    vISA_ASSERT(lr->getPhyReg() == NULL,
                "Spilled Live Range shouldn't have physical reg");
    vISA_ASSERT(lr->getSpillCost() < MAXSPILLCOST,
                "ERROR: spill live range with infinite spill cost");
    // create spill loc for holding spilled addr regs
    createNewSpillLocDeclare(dcl);
  }
  // take care of alias declares
  for (G4_Declare *dcl : kernel.Declares) {
    if (!dcl->getRegVar()->isRegAllocPartaker()) // skip non reg alloc candidate
      continue;

    if (dcl->getAliasDeclare() != NULL && // dcl is not a representative declare
        dcl->getAliasDeclare()
            ->isSpilled()) // dcl's representative decl is spilled
    {
      G4_Declare *sp = createNewSpillLocDeclare(dcl);
      // when doing RA multiple times (due to spill code), we don't want to set
      // alias information more than once
      if (sp->getAliasDeclare() == NULL) {
        sp->setAliasDeclare(dcl->getAliasDeclare()->getSpilledDeclare(),
                            dcl->getAliasOffset());
      }
    }
  }
}

bool isSpillCandidateForLifetimeOpRemoval(G4_INST *inst) {
  if (inst->isPseudoKill()) {
    return inst->getDst()->isSpilled();
  } else if (inst->isLifeTimeEnd()) {
    return inst->getSrc(0)->asSrcRegRegion()->isSpilled();
  }

  return false;
}

void SpillManager::insertSpillCode() {
  //
  // create spill locations
  //
  createSpillLocations(kernel);

  updateRMWNeeded();

  for (G4_BB *bb : kernel.fg) {
    bbId = bb->getId();
    //
    // handle spill code for the current BB
    //

    // In one iteration remove all spilled lifetime.start/end
    // ops.
    bb->erase(std::remove_if(bb->begin(), bb->end(),
                             isSpillCandidateForLifetimeOpRemoval),
              bb->end());

    for (INST_LIST_ITER inst_it = bb->begin(); inst_it != bb->end();) {
      G4_INST *inst = *inst_it;

      currCISAOffset = inst->getVISAId();

      const unsigned numSrc = inst->getNumSrc();
      std::vector<G4_Operand *> operands_analyzed(numSrc, nullptr);
      std::vector<G4_Declare *> declares_created(numSrc, nullptr);
      // insert spill inst for spilled srcs
      for (unsigned i = 0; i < numSrc; i++) {
        replaceSpilledSrc(bb, inst_it, inst, i, operands_analyzed,
                          declares_created);
      }
      // insert spill inst for spilled dst
      replaceSpilledDst(bb, inst_it, inst, operands_analyzed, declares_created);

      //
      // Process predicate
      //
      G4_Predicate *predicate = inst->getPredicate();
      if (predicate != NULL) {
        replaceSpilledPredicate(bb, inst_it, inst);
      }

      //
      // Process condMod
      //
      G4_CondMod *mod = inst->getCondMod();
      if (mod != NULL && mod->getBase() != NULL) {
        replaceSpilledFlagCondMod(bb, inst_it, inst);
      }
      inst_it++;
    }
    bbId = UINT_MAX;
  }
}
