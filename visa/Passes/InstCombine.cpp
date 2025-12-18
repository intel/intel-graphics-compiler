/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "InstCombine.hpp"

#include <functional>
#include <limits>
#include <unordered_set>

using namespace vISA;

class InstCombiner {
  IR_Builder &builder;
  FlowGraph &fg;

  // In occasions where we need to force GRF alignment of a scalar register
  // (e.g. for targeting send payloads) we don't want to blow up register
  // allocation; so we limit ourselves to just a few we should distinguish
  // between variables that are global and those that are local.
  static const int MAX_FORCE_ALIGNS = 2;
  //
  int forceAlignsLeft = MAX_FORCE_ALIGNS;

public:
  InstCombiner(IR_Builder &_builder, FlowGraph &_fg)
      : builder(_builder), fg(_fg) {}

  void run();

private:
  bool tryInstPropagate(INST_LIST_ITER iitr, INST_LIST_ITER eitr);
}; // InstCombiner

void vISA::InstCombine(IR_Builder &builder, FlowGraph &fg) {
  InstCombiner ic(builder, fg);
  ic.run();
}

void InstCombiner::run() {
  for (BB_LIST_ITER bitr = fg.begin(); bitr != fg.end(); ++bitr) {
    G4_BB *bb = *bitr;

    bb->resetLocalIds();

    INST_LIST_ITER iitr = bb->begin(), eitr = bb->end();
    while (iitr != eitr) {
      G4_INST *defInst = *iitr;
      G4_Operand *defDst = defInst->getDst();
      if (!defDst) {
        iitr++;
        continue;
      }

      builder.doConsFolding(defInst);
      builder.doSimplification(defInst);
      if (tryInstPropagate(iitr, eitr)) {
        INST_LIST_ITER tmp = iitr;
        iitr++;
        bb->erase(tmp);
      } else {
        iitr++;
      }
    } // for insts in bb
  }   // for blocks
}

// TMP = def (...) A,  B
// ...
// A = op ... (prevents us from propagating: A and B)
// ...
// ... = use ... TMP
//
// prevents us from forwarding
static bool hasAntiDependenceBetweenLastUse(INST_LIST_ITER iitr,
                                            INST_LIST_ITER eitr) {
  // find the furthest use
  G4_INST *inst = *iitr;
  G4_INST *lastUse = inst->use_front().first;
  for (USE_EDGE_LIST_ITER iter = inst->use_begin(), uend = inst->use_end();
       iter != uend; ++iter) {
    G4_INST *useInst = iter->first;
    if (useInst->getLocalId() > lastUse->getLocalId()) {
      lastUse = useInst;
    }
  }
  INST_LIST_ITER forwardIter = iitr;
  forwardIter++;
  while (forwardIter != eitr && *forwardIter != lastUse) {
    if ((*forwardIter)->isWARdep(inst)) {
      return true;
    }
    forwardIter++;
  }
  vISA_ASSERT(forwardIter != eitr, "hit end of block without finding use");
  return false;
}

// Returns true sure there is a lifetime.end for any of the sources of the
// defining inst.
//
//     add T, A, B
//     ...
//     lifetime.end A << cannot propagate A past this
//     ...
//     op ..., T, ...
//
static bool hasLifetimeEndBetweenLastUse(INST_LIST_ITER iitr,
                                         INST_LIST_ITER eitr,
                                         const G4_INST *useInst,
                                         const G4_Operand *defSrc0,
                                         const G4_Operand *defSrc1 = nullptr,
                                         const G4_Operand *defSrc2 = nullptr) {
  auto findDecl = [&](const G4_Operand *defSrc) {
    const G4_SrcRegRegion *defSrcRegRgn =
        defSrc && defSrc->isSrcRegRegion() ? defSrc->asSrcRegRegion() : nullptr;
    const G4_Declare *currInstDclSrc =
        defSrcRegRgn ? defSrcRegRgn->getBaseRegVarRootDeclare() : nullptr;
    return currInstDclSrc;
  };
  const G4_Declare *currInstDclSrc0 = findDecl(defSrc0);
  const G4_Declare *currInstDclSrc1 = findDecl(defSrc1);
  const G4_Declare *currInstDclSrc2 = findDecl(defSrc2);

  INST_LIST_ITER cpIter = iitr;
  cpIter++;
  while (*cpIter != useInst) {
    if ((*cpIter)->isLifeTimeEnd()) {
      // Check whether lifetime end is for same opnd
      const G4_Declare *lifetimeEndTopDcl =
          GetTopDclFromRegRegion((*cpIter)->getSrc(0));
      if (lifetimeEndTopDcl == currInstDclSrc0 ||
          lifetimeEndTopDcl == currInstDclSrc1 ||
          lifetimeEndTopDcl == currInstDclSrc2) {
        return true;
      }
    }
    cpIter++;
    if (cpIter == eitr) // end of block
      return false;
  }

  return false;
}

// integer folds:
//   add T, s0, s1; add *, *, T ==> add3 *, s0, s1, T
//   add T, s0, s1; add *, T, * ==> add3 *, T, s0, s1
//
//   add T, s0, immA; add *, T, immB ==> add *, s0, (immA+immB)
//   (we could do this via add3 and reduction again)
//
// TODO:
//   mul T, s0, s1 -> add *, T, * ==> mad  *, *, s0, s1
//   mul T, s0, s1 -> add *, *, T ==> mad  *, s0, s1, *
//
//   shl T, s0 << N -> add *, X, T ==> mad X, s0, 2^n
//
//   logic -> logic ==> bfn
//   add T, X, imm -> lsc untyped [T] => lsc untyped [X + imm]
//                    (given various constraints on X and imm)
bool InstCombiner::tryInstPropagate(INST_LIST_ITER iitr, INST_LIST_ITER eitr) {
  G4_INST *defInst = *iitr;
  if (!defInst->canPropagateBinaryToTernary()) {
    return false;
  } else if (defInst->use_size() == 0) {
    return false; // probably unreachable, but keep for sanity sake
  } else if (hasAntiDependenceBetweenLastUse(iitr, eitr)) {
    return false; // someone clobbers one of the def() sources before the use()
  }

  bool canFoldAdd3 = builder.getPlatform() >= Xe_XeHPSDV;

  // defer folding until we can prove all uses can be done
  std::vector<std::function<void()>> applyUses;
  //
  std::unordered_set<G4_Declare *> grfForcedAlignments;
  std::unordered_set<G4_INST *> usedAddsTargetedToAdd3;

  G4_Operand *defSrc0 = defInst->getSrc(0);
  G4_Operand *defSrc1 = defInst->getSrc(1);
  bool defIsSimd1WrEn =
      defInst->getExecSize() == 1 && defInst->isWriteEnableInst();
  // OKAY
  //     def (E)
  //     use (E)
  //
  // (W) def (1)
  //     use (E)
  //
  // (W) def (E)
  // (W) use (E)
  auto execInfoCanCanPropagate = [&](const G4_INST *useInst) {
    return defIsSimd1WrEn ||
           (defInst->isWriteEnableInst() == useInst->isWriteEnableInst() &&
            defInst->getExecLaneMask() == useInst->getExecLaneMask());
  };

  // copy def[fromDefSrcIx] to toUseInst[toUseSrcIx]
  auto copyOperand = [&](Gen4_Operand_Number fromDefSrcIx, G4_INST *toUseInst,
                         Gen4_Operand_Number toUseSrcIx) {
    G4_Operand *oldUseSrc = toUseInst->getSrc(toUseSrcIx - 1);
    if (oldUseSrc) {
      toUseInst->removeDefUse(toUseSrcIx);
    }
    G4_Operand *defSrc = defInst->getSrc(fromDefSrcIx - 1);
    G4_Operand *newUseSrc = builder.duplicateOperand(defSrc);
    toUseInst->setSrc(newUseSrc, toUseSrcIx - 1);
    // for all defs of defInst targeting defSrcIx copy those defs over
    defInst->copyDef(toUseInst, fromDefSrcIx, toUseSrcIx);
  };

  // check if each use can be combined
  for (USE_EDGE_LIST_ITER uitr = defInst->use_begin();
       uitr != defInst->use_end(); uitr++) {
    G4_INST *useInst = uitr->first;
    // copies toUseSrcA from def
    auto copyOperandsToUseSrcs = [&](Gen4_Operand_Number toUseSrcA,
                                     Gen4_Operand_Number toUseSrcB) {
      copyOperand(Opnd_src0, useInst, toUseSrcA);
      copyOperand(Opnd_src1, useInst, toUseSrcB);
    };

    if (hasLifetimeEndBetweenLastUse(iitr, eitr, useInst, defSrc0, defSrc1)) {
      return false;
    }

    auto opsAre = [&](G4_opcode defOp, G4_opcode useOp) {
      return defInst->opcode() == defOp && useInst->opcode() == useOp;
    };
    G4_Operand *useSrcOpnd = nullptr, *useOtherSrcOpnd = nullptr;
    if (uitr->second == Opnd_src0) {
      useSrcOpnd = useInst->getSrc(0);
      useOtherSrcOpnd = useInst->getSrc(1);
    } else if (uitr->second == Opnd_src1) {
      useSrcOpnd = useInst->getSrc(1);
      useOtherSrcOpnd = useInst->getSrc(0);
    } else {
      return false;
    }

    // similar criteria from G4_INST::canPropagateTo
    if (useSrcOpnd->getType() != defInst->getDst()->getType()) {
      return false; // don't bother with type conversion
    } else if (useInst->isLifeTimeEnd()) {
      return false;
    } else if (useInst->getPredicate()) {
      return false; // punt on predication (could match predicates)
    } else if (useInst->getDst() == nullptr) {
      return false; // e.g. G4_pseudo_fcall
    } else if (!execInfoCanCanPropagate(useInst)) {
      return false; // e.g. oddball ExecSize/NoMask combinations
    }

    const bool ENABLE_ADD_FOLD = false; // TODO: incrementally enable
    if (ENABLE_ADD_FOLD && opsAre(G4_add, G4_add)) {
      // see if we can reassociate the source operands
      //   add T,   s0, immA
      //   add dst, T,  immB
      // =>
      //   add dst, s0, (immA + immB)
      // (there's an older reassoc pass, but we need to do it
      // here since other folds in this pass can generate the pattern
      // and we don't want to ping/pong between these passes)
      const Gen4_Operand_Number useSrcIx = (*uitr).second;
      bool foldedChainedAdds = false;
      if ((defSrc0->isImm() || defSrc1->isImm()) && useOtherSrcOpnd->isImm()) {
        int64_t imm = useOtherSrcOpnd->asImm()->getImm();
        Gen4_Operand_Number defVarSrcIx = Opnd_src0;
        if (defSrc0->isImm()) {
          imm += defSrc0->asImm()->getImm();
          defVarSrcIx = Opnd_src1;
        } else {
          imm += defSrc1->asImm()->getImm();
          defVarSrcIx = Opnd_src0;
        }
        if (imm >= std::numeric_limits<int32_t>::min() ||
            imm <= std::numeric_limits<int32_t>::max()) {
          foldedChainedAdds = true;
          applyUses.emplace_back([&]() {
            copyOperand(defVarSrcIx, useInst, useSrcIx);
            G4_Imm *foldedImm =
                builder.createImmWithLowerType(imm, useInst->getExecType());
            unsigned ix = (useSrcIx == Opnd_src0 ? Opnd_src1 : Opnd_src0) - 1;
            useInst->setSrc(foldedImm, ix);
          });
        }
      } // chained add

      // if that fails, but we have an add3 operation then we can make
      //   add T,   s0, s1
      //   add dst, T,  s2
      // =>
      //   add3 dst, s0, s1, s2
      if (!foldedChainedAdds && canFoldAdd3) {
        if (usedAddsTargetedToAdd3.find(useInst) !=
            usedAddsTargetedToAdd3.end()) {
          // FIXME: this needs to handle folding to the same target add
          //   add  D2  = D0  D1
          //   add  ... = D2  D2
          // This will show up as dual uses (src0 and src1)
          // and will want to expand around both slots.
          // So it will become (assume use in src0 slot folds first):
          //   add3 ... =  D0  D2  D1
          // then the next application clobbers this...
          //
          // TODO: apply a better identity here (turn into a mad)
          return false;
        }
        usedAddsTargetedToAdd3.insert(useInst);
        applyUses.emplace_back([&]() {
          // promote the second add to an add3;
          // replace src2 and the transitive operand
          //   (which can be either src0 or src1)
          useInst->setOpcode(G4_add3);
          if (useSrcIx == Opnd_src1) {
            copyOperandsToUseSrcs(Opnd_src1, Opnd_src2);
            defInst->copyDef(useInst, Opnd_src1, Opnd_src1);
          } else {
            copyOperandsToUseSrcs(Opnd_src0, Opnd_src2);
          }
        });
      }
    } else if (opsAre(G4_add, G4_sends)) {
      // try to promote to LSC descriptor
      //
      if (uitr->second != Opnd_src0)
        return false; // not the address operand
      else if (builder.getPlatform() < Xe2)
        return false; // not supported in HW
                      //
      G4_InstSend *useSendInst = useInst->asSendInst();
      G4_SendDescRaw *sdr = useSendInst->getMsgDescRaw();
      if (!sdr)
        return false;
      if (!sdr->isLscOp() || sdr->getSFID() == SFID::TGM)
        return false; // must be non-TGM LSC
                      //
      int immOffset = 0;
      G4_SrcRegRegion *varOffset = nullptr;
      Gen4_Operand_Number varSrcIx = Opnd_src0;
      if (defSrc0->isSrcRegRegion() && defSrc1->isImm()) {
        immOffset = (int)defSrc1->asImm()->getImm();
        varOffset = defSrc0->asSrcRegRegion();
        varSrcIx = Opnd_src0;
      } else if (defSrc0->isImm() && defSrc1->isSrcRegRegion()) {
        immOffset = (int)defSrc0->asImm()->getImm();
        varOffset = defSrc1->asSrcRegRegion();
        varSrcIx = Opnd_src1;
      } else {
        // var/var, imm/imm, or some other nonsense
        return false;
      }
      immOffset += sdr->getLscImmOff();

      bool varDeclNeedsGrfAlignment = false;
      if (sdr->getLscDataOrder() == LSC_DATA_ORDER_TRANSPOSE) {
        // transpose accesses only work if the scalar is aligned
        auto *varDecl = varOffset->getBaseRegVarRootDeclare();
        bool isGrfAligned = varOffset->getSubRegOff() == 0 &&
                            varDecl->getByteAlignment() >= builder.getGRFSize();
        if (!isGrfAligned) {
          bool canAlign =
              forceAlignsLeft - (int)grfForcedAlignments.size() > 0 &&
              !varDecl->getRegVar()
                   ->isPhyRegAssigned() && // not already allocated
              !varDecl->isInput() &&       // inputs can't move
              !varDecl->isOutput();        // outputs can't move
          if (!canAlign) {
            return false;
          }
          // The scalar is not aligned but we can force alignment
          // * don't go nuts here, it'll burn GRFs fast
          // * we only commit to forcing alignment if we commit
          //   to the fold
          // This can work only if the source is GRF aligned.
          // We can't have the register allocator choosing some
          // misaligned register.
          //
          // Only pull the trigger if we commit to the fold
          // into all uses.
          //
          // NOTE: we should think hard about this; we could chew up
          // a ton of GRFs this away
          varDeclNeedsGrfAlignment = true;
          grfForcedAlignments.insert(varDecl);
        }
      } else if (varOffset->getRegion()->isScalar()) {
        // non-transpose
        //   e.g. broadcasting into a vector payload
        return false;
      }

      int sendAddrSize = sdr->getLscAddrSizeBytes();
      if (sendAddrSize != varOffset->getTypeSize()) {
        // e.g. must not copy an a16 address into an a32 payload without
        // widening
        //   add  tmp:d  var:w  0x10:w
        //   load.a32 [tmp] // cannot use var here!
        return false;
      } else if (!varOffset->getBase()->asRegVar()->getDeclare()->useGRF()) {
        return false;
      } else if (varOffset->isIndirect()) {
        return false;
      } else if (varOffset->hasModifier()) {
        return false;
      } else if (!sdr->canSetLscImmOff(immOffset)) {
        return false; // e.g. offset too large or misaligned
      }

      // ensure not turned off by options
      auto addrType = sdr->getLscAddrType();
      uint32_t enabledAddrTypes =
          builder.getuint32Option(vISA_lscEnableImmOffsFor);
      bool isEnabledAddrType =
          (enabledAddrTypes & (1 << getLscImmOffOpt(addrType))) != 0;

      if (!builder.getOption(vISA_lscEnableImmOffsetForA32Stateful)) {
        bool isLscA32Stateful = [](LSC_ADDR_TYPE AddrTy) {
          switch (AddrTy) {
          case LSC_ADDR_TYPE_BSS:
          case LSC_ADDR_TYPE_SS:
          case LSC_ADDR_TYPE_BTI:
            return true;
          default:
            break;
          }
          return false;
        } (addrType);

        if (sdr->isSLM() || isLscA32Stateful)
          isEnabledAddrType = false;
      }

      if (!isEnabledAddrType) {
        return false;
      }

      applyUses.emplace_back([this, copyOperand, varSrcIx, useSendInst, sdr, immOffset, varDeclNeedsGrfAlignment, varOffset]() {
        copyOperand(varSrcIx, useSendInst, Opnd_src0);
        sdr->setLscImmOff(immOffset);
        // this is really nuts:
        //   send instructions has a G4_SendDesc field as well
        //   as G4_Operand (G4_Imm or G4_Reg for a0.#)
        // we have update the immediate operand src3
        auto *exDesc = useSendInst->getSrc(3);
        if (exDesc->isImm()) {
          G4_Imm *newExDesc =
              builder.createImm(sdr->getExtendedDesc(), Type_UD);
          useSendInst->setSrc(newExDesc, 3);
        }
        if (varDeclNeedsGrfAlignment) {
          varOffset->getBaseRegVarRootDeclare()->setSubRegAlign(ThirtyTwo_Word);
        }
      });
      // LSC send immoff promote
    } else {
      // unsupported pattern
      return false;
    }
  } // for uses

  // commit the changes
  for (auto &apply : applyUses) {
    apply();
  }

  // unlink our def and use pairs (defInst is being removed)
  defInst->removeDefUse(Opnd_src0);
  defInst->removeDefUse(Opnd_src1);
  forceAlignsLeft -= (int)grfForcedAlignments.size();

  return true;
}
