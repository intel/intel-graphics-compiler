/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "G4_Verifier.hpp"

#include <sstream>

using namespace vISA;

void verifyG4Kernel(G4_Kernel &k, Optimizer::PassIndex index, bool alwaysOn,
                    G4Verifier::VerifyControl ctrl) {
  if (alwaysOn || k.fg.builder->getOption(VISA_FullIRVerify)) {
    G4Verifier verifier(k, ctrl, index);
    verifier.verify();
  }
}

void verifyG4Inst(G4_Kernel &kernel, G4_INST *inst,
                  Optimizer::PassIndex index) {
  G4Verifier verifier(kernel, G4Verifier::VC_ASSERT, index);
  verifier.verifyInst(inst);
}

std::atomic<int> G4Verifier::index(0);

G4Verifier::G4Verifier(G4_Kernel &k, VerifyControl ctrl,
                       Optimizer::PassIndex index)
    : kernel(k), verifyCtrl(ctrl), passIndex(index) {
  if (ctrl == VC_AppendDump || ctrl == VC_NewDump) {
    const char *buf = nullptr;
    k.getOptions()->getOption(VISA_AsmFileName, buf);
    std::string dumpName;
    if (buf != nullptr) {
      dumpName = std::string(buf);
    }
    dumpName += ".g4verify.dump.txt";
    if (ctrl == VC_AppendDump)
      dumpText.open(dumpName, std::ofstream::app);
    else
      dumpText.open(dumpName, std::ofstream::trunc);
  }
}

void G4Verifier::verify() {
  // For each instruction do verification.
  for (auto BBI = kernel.fg.cbegin(), BBE = kernel.fg.cend(); BBI != BBE;
       ++BBI) {
    auto bb = *BBI;
    if (!bb->getFuncInfo() || !bb->getFuncInfo()->contains(bb))
      vISA_ASSERT(false, "mismatch in bb->funcInfo link");
    if (bb->isEndWithCall()) {
      vISA_ASSERT(bb->getFuncInfo() != bb->Succs.front()->getFuncInfo(),
                  "caller and callee have same FuncInfo*");
    }
    if (bb->getBBType() & G4_BB_EXIT_TYPE) {
      vISA_ASSERT(bb->getFuncInfo() != bb->Succs.front()->getFuncInfo(),
                  "return and returnee have same FuncInfo*");
    }
    if (bb->isSpecialEmptyBB()) {
      // Special empty bb should have a single label instruction.
      vISA_ASSERT(bb->size() == 1 && bb->front()->isLabel(),
                  "illegal special basic block");
    }

    for (auto I = bb->begin(), E = bb->end(); I != E; ++I) {
      G4_INST *inst = *I;
      verifyInst(inst);
    }
  }
}

bool G4Verifier::verifyInst(G4_INST *inst) {
  vISA_ASSERT(inst != NULL, "null instruction unexpected");
  if (inst) {
    verifyOpcode(inst);
    verifyOpnd(inst->getDst(), inst);
    verifyOpnd(inst->getSrc(0), inst);
    verifyOpnd(inst->getSrc(1), inst);
    verifyOpnd(inst->getSrc(2), inst);
    verifyOpnd(inst->getPredicate(), inst);
    verifyOpnd(inst->getCondMod(), inst);
    verifyOpnd(inst->getImplAccDst(), inst);
    verifyOpnd(inst->getImplAccSrc(), inst);

    if (inst->isSend()) {
      verifySend(inst);
    } else if (inst->isDpas()) {
      verifyDpas(inst);
    } else if (inst->isLfsr()) {
      verifyLfsr(inst);
    } else if (inst->isDnscl()) {
      verifyDnscl(inst);
    }
    verifyAccMov(inst);

    verifyDstSrcOverlap(inst);

    if (passIndex == Optimizer::PI_cleanMessageHeader ||
        passIndex == Optimizer::PI_forceNoMaskOnM0 ||
        passIndex == Optimizer::PI_renameRegister ||
        passIndex == Optimizer::PI_localDefHoisting ||
        passIndex == Optimizer::PI_localCopyPropagation ||
        passIndex == Optimizer::PI_localInstCombine ||
        passIndex == Optimizer::PI_reassociateConst ||
        passIndex == Optimizer::PI_cselPeepHoleOpt ||
        passIndex == Optimizer::PI_dce) {
      // def-use chain should be valid after these passes
      return verifyDefUseChain(inst);
    }

    if (passIndex == Optimizer::PI_HWConformityChk ||
        passIndex == Optimizer::PI_addSWSBInfo) {
      // feature verification. Do it twice for now.
      verifyBFMixedMode(inst);
    }
  }
  return true;
}

// Returns true if this use is defined by the defInst (dst, condMod, or acc)
// Otherwise returns false.
static bool checkDefUse(G4_INST *defInst, G4_Operand *use) {
  if (!use)
    return false;

  G4_Operand *dst = defInst->getOperand(Opnd_dst);
  G4_Operand *condMod = defInst->getOperand(Opnd_condMod);

  if (use->isAccReg()) {
    // use is acc
    // ToDo: we should check if acc is re-defined in between as well
    if (defInst->getImplAccDst() != NULL || dst->isAccReg()) {
      return true;
    }
  }

  if (dst && Rel_disjoint != use->compareOperand(dst, defInst->getBuilder()))
    return true;

  if (condMod &&
      Rel_disjoint != use->compareOperand(condMod, defInst->getBuilder()))
    return true;

  return false;
}

bool G4Verifier::verifyDefUseChain(G4_INST *inst) {
  bool isValid = true;

  for (auto I = inst->use_begin(), E = inst->use_end(); I != E; ++I) {
    auto DU = *I;
    // A valid def-use satisfies
    //
    // inst[dst/condMod] defines DU.first[DU.second]
    //
    G4_Operand *use = (DU.first)->getOperand(DU.second);
    if (!checkDefUse(inst, use)) {
      isValid = false;
      printDefUse(inst, DU.first, DU.second);
      assertIfEnable();
    }
  }

  for (auto I = inst->def_begin(), E = inst->def_end(); I != E; ++I) {
    auto UD = *I;
    // A valid use-def satisfies
    //
    // UD.first[dst/condMod] defines inst[UD.second]
    //
    G4_Operand *use = inst->getOperand(UD.second);
    if (!checkDefUse(UD.first, use)) {
      isValid = false;
      printDefUse(UD.first, inst, UD.second);
      assertIfEnable();
    }
  }

  return isValid;
}

void G4Verifier::printDefUseImpl(std::ostream &os, G4_INST *def, G4_INST *use,
                                 Gen4_Operand_Number pos) {
  os << "\n  def: ";
  def->emit(os);
  os << "\n user: ";
  use->emit(os);
  os << "\n opnd: ";
  if (use->getOperand(pos)) {
    use->getOperand(pos)->emit(os);
  }
}

/// Dump or warn def-use.
void G4Verifier::printDefUse(G4_INST *def, G4_INST *use,
                             Gen4_Operand_Number pos) {
  if (dumpText.is_open() && dumpText.good()) {
    dumpText << "\n\nIndex: " << index++;
    printDefUseImpl(dumpText, def, use, pos);
  } else if (verifyCtrl == VC_WARN) {
    std::cerr << "\n\nInvalid def-use pair detected!!\n";
    printDefUseImpl(std::cerr, def, use, pos);
  }
}

void G4Verifier::assertIfEnable() const {
  vISA_ASSERT(false, "G4Verification failure");
}

bool G4Verifier::dataHazardCheck(G4_Operand *dst, G4_Operand *src) {
  G4_RegVar *dstVar =
      static_cast<G4_RegVar *>(dst->asDstRegRegion()->getBase());
  G4_RegVar *srcVar =
      static_cast<G4_RegVar *>(src->asSrcRegRegion()->getBase());
  if (!dstVar->isRegVar() || !dstVar->isGreg() || !srcVar->isRegVar() ||
      !srcVar->isGreg()) {
    return false;
  }

  int dstStart = dst->getLinearizedStart();
  int dstEnd = dst->getLinearizedEnd();
  int srcStart = src->getLinearizedStart();
  int srcEnd = src->getLinearizedEnd();

  if (dstEnd < srcStart || srcEnd < dstStart) {
    return false;
  }

  int dstReg = dstStart / kernel.numEltPerGRF<Type_UB>();
  int dstRegNum = (dstEnd - dstStart + kernel.numEltPerGRF<Type_UB>()) /
                  kernel.numEltPerGRF<Type_UB>();
  int srcReg = srcStart / kernel.numEltPerGRF<Type_UB>();
  int srcRegNum = (srcEnd - srcStart + kernel.numEltPerGRF<Type_UB>()) /
                  kernel.numEltPerGRF<Type_UB>();
  int srcReg2 = -1;

  if (srcRegNum > 1) {
    srcReg2 = srcReg + 1;
  }

  if (dstRegNum >= 2 && srcRegNum == 1) {
    srcReg2 = srcReg;
  }

  if (dstReg == srcReg2) {
    return true;
  }

  return false;
}

void G4Verifier::verifyDstSrcOverlap(G4_INST *inst) {
  if (passIndex == Optimizer::PI_regAlloc &&
      kernel.fg.builder->avoidDstSrcOverlap()) {
    G4_DstRegRegion *dst = inst->getDst();

    if (inst->isSend() || dst == NULL || dst->isNullReg() ||
        inst->opcode() == G4_madm) {
      return;
    }

    if (!inst->isComprInst()) {
      return;
    }

    int dstStart = dst->getLinearizedStart() / kernel.numEltPerGRF<Type_UB>();
    int dstEnd = dst->getLinearizedEnd() / kernel.numEltPerGRF<Type_UB>();

    for (int i = 0; i < inst->getNumSrc(); i++) {
      G4_Operand *src = inst->getSrc(i);
      if (inst->isDpas() && i != 1) {
        continue;
      }
      if (src != NULL && !src->isNullReg() && src->getTopDcl() &&
          (src->getTopDcl()->getRegFile() == G4_GRF ||
           src->getTopDcl()->getRegFile() == G4_INPUT)) {
        [[maybe_unused]] bool overlap = dataHazardCheck(dst, src);

        int srcStart =
            src->getLinearizedStart() / kernel.numEltPerGRF<Type_UB>();
        int srcEnd = src->getLinearizedEnd() / kernel.numEltPerGRF<Type_UB>();
        if (dstEnd != dstStart ||
            srcStart != srcEnd) // Any operand is more than 2 GRF
        {
          vISA_ASSERT(!overlap, "dst and src0 overlap");
        }
      }
    }
  }
}

void G4Verifier::verifySend(G4_INST *inst) {
  vISA_ASSERT(inst->isSend(), "expect send inst");
  if (passIndex == Optimizer::PI_regAlloc) {
    G4_DstRegRegion *dst = inst->getDst();
    G4_SrcRegRegion *src0 = inst->getSrc(0)->asSrcRegRegion();
    G4_SrcRegRegion *src1 =
        inst->isSplitSend() ? inst->getSrc(1)->asSrcRegRegion() : nullptr;

    bool checkEoT = inst->isEOT() && kernel.fg.builder->hasEOTGRFBinding();
    if (checkEoT) {
        [[maybe_unused]] auto checkEOTSrc = [this](G4_SrcRegRegion *src) {
        // Send instruction's header and payload must be placed in last 16 GRFs if the EOT flag is set.
        const unsigned int EOTStart = (kernel.getNumRegTotal()-16) * kernel.numEltPerGRF<Type_UB>();
        if (src->isNullReg()) {
          return true;
        }
        return src->getLinearizedStart() >= EOTStart;
      };

      if (kernel.getNumRegTotal() >= 128) {
        vISA_ASSERT(checkEOTSrc(src0),
                    "src0 for EOT send is not in last 16 GRFs");
        if (src1 != nullptr) {
          vISA_ASSERT(checkEOTSrc(src1),
                      "src1 for EOT sends is not in last 16 GRFs");
        }
      }
    }

    if (inst->isSplitSend() && kernel.fg.builder->noSrc0Src1OverlapSend()) {
      // simd1 split send is allowed to have srcs overlap. When it's simd1,
      // overlap for the rest of the payload shouldn't matter
      bool allowSrcOverlap = inst->getExecSize() == g4::SIMD1;
      if (!allowSrcOverlap && src0->getBase()->isGreg() && src1 &&
          src1->getBase()->isGreg()) {
        int src0Start =
            src0->getLinearizedStart() / kernel.numEltPerGRF<Type_UB>();
        int src0End = src0Start + inst->getMsgDesc()->getSrc0LenRegs() - 1;
        int src1Start =
            src1->getLinearizedStart() / kernel.numEltPerGRF<Type_UB>();
        int src1End = src1Start + inst->getMsgDesc()->getSrc1LenRegs() - 1;
        [[maybe_unused]] bool noOverlap = src0End < src1Start || src1End < src0Start;
        vISA_ASSERT(noOverlap, "split send src0 and src1 overlap");
      }
    }

    if (kernel.fg.builder->WaDisableSendSrcDstOverlap()) {
      if (!dst->isNullReg()) {
        if (src0->getBase()->isGreg()) {
          [[maybe_unused]] bool noOverlap =
              dst->getLinearizedEnd() < src0->getLinearizedStart() ||
              src0->getLinearizedEnd() < dst->getLinearizedStart();
          vISA_ASSERT(noOverlap, "send dst and src0 overlap");
        }
        if (src1 && !src1->isNullReg()) {
          [[maybe_unused]] bool noOverlap =
              dst->getLinearizedEnd() < src1->getLinearizedStart() ||
              src1->getLinearizedEnd() < dst->getLinearizedStart();
          vISA_ASSERT(noOverlap, "split send dst and src1 overlap");
        }
      }
    }
    if (kernel.getNumRegTotal() == 512) {
      // verify that r511 is not used as a source for sendgx/sendgxc.
      [[maybe_unused]] auto checkNotLastGRF = [this](G4_Operand *opnd) {
        auto r511InBytes = 511 * kernel.numEltPerGRF<Type_UB>();
        if (opnd->isNullReg())
          return true;
        return opnd->getLinearizedStart() < r511InBytes;
      };
      vISA_ASSERT(checkNotLastGRF(dst), "dst can't be r511 for sendgx");
      vISA_ASSERT(checkNotLastGRF(src0), "src0 can't be r511 for sendgx");
      if (src1)
        vISA_ASSERT(checkNotLastGRF(src1), "src1 can't be r511 for sendgx");
    }
  }
}

void G4Verifier::verifyOpnd(G4_Operand *opnd, G4_INST *inst) {
  if (inst->isDpas()) {
    // Temporarily skip for now
    return;
  }

  uint8_t execSize = inst->getExecSize();

  if (opnd == NULL) {
    return;
  }

  if (inst->opcode() == G4_sel && opnd->isCondMod()) {
    // conditional modifier for sel is a don't care, so we can skip verification
    return;
  }

  // Skip verifying operands of an intrinsic as it usually does not follow
  // region rules.
  if (inst->isIntrinsic())
    return;

  // FIXME: If isImm() condition is removed then some assertions are hit.
  // This means somewhere in Jitter operand sharing is happening for
  // immediate type operands. This should be fixed.
  // For Imm, AddrExp, AddrExpList, Labels, hashtable lookup is
  // performed at creation time unline SrcRegion, DstRegion,
  // Predicate, CondMod. This means former type of operands
  // can be shared across instructions.
  if (opnd->getInst() != inst && opnd->isLabel() == false &&
      opnd->isImm() == false && opnd->isNullReg() == false &&
      opnd->isAddrExp() == false) {
    DEBUG_VERBOSE("operand does not have exactly one owning instruction "
                  "(shared or orphaned)");

    std::cerr << "operand: ";
    opnd->emit(std::cerr);
    std::cerr << " in instruction:\n  ";
    inst->emit(std::cerr);
    std::cerr << "\n";
    std::cerr << "   operand: ";
    opnd->emit(std::cerr);
    std::cerr << "\n";

    if (opnd->getInst() == NULL) {
      DEBUG_VERBOSE("operand has no owner instruction (orphaned)");
      vISA_ASSERT(false, "operand has no owner instruction (orphaned)");
    } else {
      DEBUG_VERBOSE("operand pointer is shared by another instruction");
      vISA_ASSERT(false, "operand pointer is shared by another instruction");
    }
    DEBUG_VERBOSE("\n");
  }

  if (inst->isSend()) {
    // send dst/src may not be GRF-aligned before HW conformity,
    // so we only check their bound in RA
    if (passIndex != Optimizer::PI_regAlloc) {
      return;
    }

    if (opnd == inst->getDst()) {
      if (opnd->isRightBoundSet() && !opnd->isNullReg()) {
        unsigned int correctRB = opnd->asDstRegRegion()->getRegOff() *
                                     kernel.numEltPerGRF<Type_UB>();
        uint32_t dstLenBytes = inst->getMsgDesc()->getDstLenBytes();
        if (dstLenBytes < kernel.getGRFSize()) {
          correctRB += (dstLenBytes - 1);
        } else {
          uint32_t correctDstLenBytes =
              std::min(opnd->getTopDcl()->getByteSize(), dstLenBytes);
          correctRB += (correctDstLenBytes - 1);
        }

        G4_Declare *parentDcl = opnd->getBase()->asRegVar()->getDeclare();
        while (parentDcl) {
          correctRB += parentDcl->getAliasOffset();
          parentDcl = parentDcl->getAliasDeclare();
        }

        if (opnd->getRightBound() != correctRB) {
          DEBUG_VERBOSE("Right bound mismatch for send inst dst. Orig rb = "
                        << opnd->getRightBound()
                        << ", correct rb = " << correctRB << "\n");

          inst->emit(std::cerr);
          DEBUG_VERBOSE("\n");
          vISA_ASSERT(false, "Right bound mismatch!");
        }
      }
    } else if (opnd == inst->getSrc(0) ||
               (inst->isSplitSend() && opnd == inst->getSrc(1))) {
      if (opnd->isRightBoundSet()) {
        unsigned int correctRB = opnd->asSrcRegRegion()->getRegOff() *
                                 kernel.numEltPerGRF<Type_UB>();
        unsigned int srcBytes = (opnd == inst->getSrc(0))
                                    ? inst->getMsgDesc()->getSrc0LenBytes()
                                    : inst->getMsgDesc()->getSrc1LenBytes();
        if (srcBytes < kernel.numEltPerGRF<Type_UB>()) {
          correctRB += (srcBytes - 1);
        } else {
          uint32_t correctSrcBytes =
              std::min(opnd->getTopDcl()->getByteSize(), srcBytes);
          correctRB += (correctSrcBytes - 1);
        }

        G4_Declare *parentDcl = opnd->getBase()->asRegVar()->getDeclare();
        while (parentDcl) {
          correctRB += parentDcl->getAliasOffset();
          parentDcl = parentDcl->getAliasDeclare();
        }

        if (opnd->getRightBound() != correctRB) {
          DEBUG_VERBOSE("Right bound mismatch for send inst src0. Orig rb = "
                        << opnd->getRightBound()
                        << ", correct rb = " << correctRB << "\n");

          inst->emit(std::cerr);
          DEBUG_VERBOSE("\n");
          vISA_ASSERT(false, "Right bound mismatch!");
        }
      }
    }
  } else {
    // Only valid ARF type are NULL and Accumulator for ternary instructions
    if (passIndex == Optimizer::PI_HWConformityChk && inst->getNumSrc() == 3) {
      if (opnd->isAreg() && !opnd->isNullReg() && !opnd->isAccReg() &&
          (inst->getPlatform() <= Xe3) &&
          !(opnd == inst->getSrc(0) && opnd->isSrReg()))
        vISA_ASSERT(false, "Not allowed ARF in ternary instruction");
    }

    if (opnd->isSrcRegRegion() && opnd->isRightBoundSet()) {
      G4_SrcRegRegion newRgn(*(opnd->asSrcRegRegion()));

      newRgn.setInst(inst);
      newRgn.computeLeftBound(*kernel.fg.builder);
      newRgn.computeRightBound(execSize);

      if (kernel.fg.builder->supportNativeSIMD32() &&
          inst->getExecSize() == g4::SIMD32 && opnd->getTypeSize() == 8) {
        [[maybe_unused]] bool indirect1x1 = opnd->isIndirect()  &&
                           !opnd->asSrcRegRegion()->getRegion()->isRegionWH();
        vISA_ASSERT(!indirect1x1,
                    "Must not be indirect 1x1 addressing mode for SIMD32 "
                    "instructions with 64b datatypes");
        vISA_ASSERT((opnd->getRightBound() - opnd->getLeftBound()) <
                        (4u * kernel.numEltPerGRF<Type_UB>()),
                    "Src cannot span more than 4 GRFs!");
      } else if ((opnd->getRightBound() - opnd->getLeftBound()) >
                 (2u * kernel.numEltPerGRF<Type_UB>())) {
        if (!(inst->opcode() == G4_pln && inst->getSrc(1) == opnd)) {
          DEBUG_VERBOSE(
              "Difference between left/right bound is greater than 2 GRF for "
              "src region. Single non-send opnd cannot span 2 GRFs. lb = "
              << opnd->getLeftBound() << ", rb = " << opnd->getRightBound()
              << "\n");
          inst->emit(std::cerr);
          DEBUG_VERBOSE("\n");
          vISA_ASSERT(false, "Left/right bound span incorrect!");
        }
      }

      if (inst->opcode() == G4_pln && inst->getSrc(1) == opnd) {
        // For pln, src1 uses 2 GRFs if exec size <= 8
        // and 4 GRFs if exec size == 16
        newRgn.computeRightBound(inst->getExecSize() > g4::SIMD8
                                     ? inst->getExecSize()
                                     : G4_ExecSize(inst->getExecSize() * 2));

        if (inst->getExecSize() > g4::SIMD8) {
          newRgn.setRightBound(newRgn.getRightBound() * 2 -
                               newRgn.getLeftBound() + 1);
        }
      }

      if (inst->getMaskOffset() > 0 && opnd == inst->getImplAccSrc()) {
        // Update left/right bound as per inst mask offset, eg Q2
        // has offset 8
        G4_Type extype;
        int extypesize;
        unsigned int multiplicationFactor = 1;
        if (opnd->isAccReg()) {
          // Right bound granularity is in terms of
          // bytes for Acc registers
          multiplicationFactor = 4;
        }

        extype = inst->getOpExecType(extypesize);
        if ((IS_WTYPE(extype) || IS_DTYPE(extype))) {
          // This condition is a result of HW Conformity requirement
          // that for exec type = D/DW, only acc0 is used even when
          // qtr control is set to Q2/H2
          newRgn.setLeftBound(0);
          newRgn.setRightBound(31);
        } else {
          newRgn.setLeftBound(newRgn.getLeftBound() +
                              (inst->getMaskOffset() * multiplicationFactor));
          newRgn.setRightBound(newRgn.getRightBound() +
                               (inst->getMaskOffset() * multiplicationFactor));
        }
      }

      if (opnd->getLeftBound() != newRgn.getLeftBound()) {
        DEBUG_VERBOSE(
            "Left bound mismatch for src opnd for following inst. Orig lb = "
            << opnd->getLeftBound()
            << ", recomputed lb = " << newRgn.getLeftBound() << "\n");
        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "Left bound mismatch!");
      }

      if (opnd->getRightBound() != newRgn.getRightBound()) {
        DEBUG_VERBOSE(
            "Right bound mismatch for src opnd for following inst. Orig rb = "
            << opnd->getRightBound()
            << ", recomputed rb = " << newRgn.getRightBound() << "\n");

        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "Right bound mismatch!");
      }
    } else if (opnd->isDstRegRegion() && opnd->isRightBoundSet() &&
               !opnd->isNullReg()) {
      G4_DstRegRegion newRgn(*(opnd->asDstRegRegion()));
      newRgn.setInst(inst);
      newRgn.computeLeftBound(*kernel.fg.builder);
      newRgn.computeRightBound(execSize);

      if (kernel.fg.builder->supportNativeSIMD32() &&
          inst->getExecSize() == g4::SIMD32 &&
          (opnd->getTypeSize() == 8)) {
        vISA_ASSERT(!opnd->isIndirect(),
                    "Must be direct addressing mode for SIMD32 instructions "
                    "with 64b datatypes");
        vISA_ASSERT((opnd->getRightBound() - opnd->getLeftBound()) <
                        (4u * kernel.numEltPerGRF<Type_UB>()),
                    "Dst cannot span more than 4 GRFs!");
      } else if ((opnd->getRightBound() - opnd->getLeftBound()) >
                     (2u * kernel.numEltPerGRF<Type_UB>()) &&
                 (inst->opcode() != G4_madw && inst->opcode() != G4_mullh)) {
        DEBUG_VERBOSE(
            "Difference between left/right bound is greater than 2 GRF for dst "
            "region. Single non-send opnd cannot span 2 GRFs. lb = "
            << opnd->getLeftBound() << ", rb = " << opnd->getRightBound()
            << "\n");
        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "Left/right bound span incorrect!");
      }

      if (!(kernel.fg.builder->hasSimplifiedRegions() ||
            kernel.fg.builder->getOption(vISA_GAReArchBugFix)) &&
          inst->getMaskOffset() > 0 && opnd == inst->getImplAccDst()) {
        // Update left/right bound as per inst mask offset, eg Q2
        // has offset 8
        G4_Type extype;
        int extypesize;
        unsigned int multiplicationFactor = 1;
        if (opnd->isAccReg()) {
          // Right bound granularity is in terms of
          // bytes for Acc registers
          multiplicationFactor = 4;
        }

        extype = inst->getOpExecType(extypesize);

        if ((IS_WTYPE(extype) || IS_DTYPE(extype))) {
          // This condition is a result of HW Conformity requirement
          // that for exec type = D/DW, only acc0 is used even when
          // qtr control is set to Q2/H2
          newRgn.setLeftBound(0);
          newRgn.setRightBound(31);
        } else {
          newRgn.setLeftBound(newRgn.getLeftBound() +
                              (inst->getMaskOffset() * multiplicationFactor));
          newRgn.setRightBound(newRgn.getRightBound() +
                               (inst->getMaskOffset() * multiplicationFactor));
        }
      }

      if (opnd->getLeftBound() != newRgn.getLeftBound()) {
        DEBUG_VERBOSE(
            "Left bound mismatch for dst opnd for following inst. Orig lb = "
            << opnd->getLeftBound()
            << ", recomputed lb = " << newRgn.getLeftBound() << "\n");

        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "Left bound mismatch");
      }

      if (opnd->getRightBound() != newRgn.getRightBound()) {
        DEBUG_VERBOSE(
            "Right bound mismatch for dst opnd for following inst. Orig rb = "
            << opnd->getRightBound()
            << ", recomputed rb = " << newRgn.getRightBound() << "\n");

        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "Right bound mismatch!");
      }
    } else if (opnd->isPredicate() && opnd->isRightBoundSet()) {
      G4_Predicate newRgn(*(opnd->asPredicate()));

      newRgn.setLeftBound(0);
      newRgn.computeRightBound(execSize);

      if (inst->getMaskOffset() > 0) {
        // Update left/right bound as per inst mask offset, eg Q2
        // has offset 8
        newRgn.setLeftBound(newRgn.getLeftBound() + inst->getMaskOffset());
        newRgn.setRightBound(newRgn.getRightBound() + inst->getMaskOffset());
      }

      if (opnd->getLeftBound() != newRgn.getLeftBound()) {
        DEBUG_VERBOSE(
            "Left bound mismatch for pred opnd for following inst. Orig lb = "
            << opnd->getLeftBound()
            << ", recomputed lb = " << newRgn.getLeftBound() << "\n");

        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "Left bound mismatch");
      }

      if (opnd->getRightBound() != newRgn.getRightBound()) {
        DEBUG_VERBOSE(
            "Right bound mismatch for pred opnd for following inst. Orig rb = "
            << opnd->getRightBound()
            << ", recomputed rb = " << newRgn.getRightBound() << "\n");

        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "Right bound mismatch!");
      }
    } else if (opnd->isCondMod() && opnd->isRightBoundSet()) {
      G4_CondMod newRgn(*(opnd->asCondMod()));

      newRgn.setLeftBound(0);
      newRgn.computeRightBound(execSize);

      if (inst->getMaskOffset() > 0) {
        // Update left/right bound as per inst mask offset, eg Q2
        // has offset 8
        newRgn.setLeftBound(newRgn.getLeftBound() + inst->getMaskOffset());
        newRgn.setRightBound(newRgn.getRightBound() + inst->getMaskOffset());
      }

      if (opnd->getLeftBound() != newRgn.getLeftBound()) {
        DEBUG_VERBOSE("Left bound mismatch for cond mod opnd for following "
                      "inst. Orig lb = "
                      << opnd->getLeftBound()
                      << ", recomputed lb = " << newRgn.getLeftBound() << "\n");

        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "Left bound mismatch");
      }

      if (opnd->getRightBound() != newRgn.getRightBound()) {
        DEBUG_VERBOSE("Right bound mismatch for cond mod opnd for following "
                      "inst. Orig rb = "
                      << opnd->getRightBound() << ", recomputed rb = "
                      << newRgn.getRightBound() << "\n");

        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "Right bound mismatch!");
      }
    } else {
      // Not implemented
    }

    if (passIndex == Optimizer::PI_regAlloc) {
      // alignment checks that can only be performed post RA
      bool threeSrcAlign16 = (inst->getNumSrc() == 3) && !inst->isSend() &&
                             !kernel.fg.builder->hasAlign1Ternary();
      bool nonScalar =
          (opnd->isSrcRegRegion() && !opnd->asSrcRegRegion()->isScalar()) ||
          (opnd->isDstRegRegion() && inst->getExecSize() > g4::SIMD2);
      bool isAssigned = opnd->isRegRegion() && opnd->getBase()->isRegVar() &&
                        opnd->getBase()->asRegVar()->isPhyRegAssigned();
      // allow replicated DF source opnd with <2;2,0> region
      bool isReplicated =
          (opnd->getType() == Type_DF) && opnd->isSrcRegRegion() &&
          (opnd->asSrcRegRegion()->getRegion()->width == 2) &&
          (opnd->asSrcRegRegion()->getRegion()->horzStride == 0) &&
          (opnd->asSrcRegRegion()->getRegion()->vertStride == 2);
      if (threeSrcAlign16 && nonScalar && isAssigned &&
          opnd->getLinearizedStart() % 16 != 0 && !isReplicated) {
        vISA_ASSERT(false,
                     "dp2/dp3/dp4/dph and non-scalar 3src op must be align16!");
      }

      // check acc source alignment
      // for explicit acc source, it and the inst's dst should both be
      // oword-aligned for implicit acc source, its subreg offset should be
      // identical to that of the dst
      if (opnd->isAccReg()) {
        [[maybe_unused]] uint32_t offset = opnd->getLinearizedStart() % 32;
        if (inst->getDst()) {
          [[maybe_unused]] uint32_t dstOffset = inst->getDst()->getLinearizedStart() % 32;
          if (opnd == inst->getImplAccSrc()) {
            vISA_ASSERT(offset == dstOffset,
                   "implicit acc source must have identical offset as dst");
          } else if (opnd->isSrcRegRegion()) {
            vISA_ASSERT((offset % 16 == 0 && dstOffset % 16 == 0),
                   "explicit acc source and its dst must be oword-aligned");
          }
        }
      }

      // if src0 is V/UV/VF imm, dst must be 16 byte aligned.
      if (inst->opcode() == G4_mov && IS_VTYPE(inst->getSrc(0)->getType())) {
        auto dst = inst->getDst();
        // should we assert if dst is not phyReg assigned?
        if (dst) {
          bool dstIsAssigned = dst->getBase()->isRegVar() &&
                               dst->getBase()->asRegVar()->isPhyRegAssigned();
          if (dstIsAssigned && dst->getLinearizedStart() % 16 != 0) {
            vISA_ASSERT(false, "destination of move instruction with V/VF imm is "
                            "not 16-byte aligned");
          }
        }
      }

      // check if the oprands with mme are GRF-aligned.
      if (opnd->isGreg() && opnd->getAccRegSel() != ACC_UNDEFINED) {
        vISA_ASSERT(opnd->getLinearizedStart() % kernel.numEltPerGRF<Type_UB>() ==
                   0,
               "operand with mme must be GRF-aligned");
      }
    }
    if (passIndex == Optimizer::PI_HWConformityChk &&
        inst->getPlatform() > Xe3) {
      if (inst->getNumSrc() >= 3) {
        vISA_ASSERT(!opnd->isIndirect(),
                    "operand must not be indirect for 3-src instructions");
      } else {
        if (opnd == inst->getSrc(1) && opnd->isIndirect()) {
          vISA_ASSERT(!inst->isMath(),
                      "indirect is not allowed by math instructions");
          vISA_ASSERT(opnd->isScalarSrc(), "indirect src1 can be only scalar");
        }

        if (opnd == inst->getSrc(0) && opnd->isIndirect()) {
          vISA_ASSERT(!inst->isMath(),
                      "indirect is not allowed by math instructions");
          if (opnd->asSrcRegRegion()->getRegion()->isRegionWH()) {
            vISA_ASSERT(inst->isIntegerPipeInstructionXe(),
                        "Vx1/VxH indirect addressing mode can be only for int "
                        "pipeline");
            vISA_ASSERT(!IS_QTYPE(opnd->getType()) &&
                            !IS_BTYPE(opnd->getType()),
                        "Q/B datatypes are not supported in Vx1/VxH indirect "
                        "addressing modes");
          }
        }
      }
    }
  }
}

void verifyLifetimeConsistency(G4_BB *bb) {
  // Verify whether misplaced pseudo_kill/lifetime.end is seen in BB
  // Following code patterns are incorrect:
  // mov (1) A,
  // ...
  // pseudo_kill A
  // As per VISA spec, we allow a single instance of pseudo_kill per
  // variable. Later RA's liveness may insert multiple. This will
  // not be invoked after RA anyway. As a precaution, we return
  // if no unassigned register is found.
  //
  // Similarly for lifetime.end
  // lifetime.end A
  // ...
  // mov (1) A,
  // This is illegal because lifetime.end appears before last use
  // in BB
  bool unassignedFound = false;

  for (INST_LIST_ITER it = bb->begin(), end = bb->end(); it != end; it++) {
    G4_INST *curInst = (*it);

    std::stack<G4_Operand *> opnds;
    opnds.push(curInst->getDst());
    opnds.push(curInst->getSrc(0));
    opnds.push(curInst->getSrc(1));
    opnds.push(curInst->getSrc(2));
    opnds.push(curInst->getPredicate());
    opnds.push(curInst->getCondMod());

    while (!opnds.empty()) {
      G4_Operand *curOpnd = opnds.top();
      opnds.pop();

      if (curOpnd != NULL && curOpnd->getTopDcl() != NULL) {
        G4_Declare *topdcl = curOpnd->getTopDcl();

        if (topdcl->getRegVar() && !topdcl->getRegVar()->isPhyRegAssigned()) {
          unassignedFound = true;
        }
      }
    }
  }

  if (unassignedFound == true) {
    typedef std::map<G4_Declare *, std::pair<G4_INST *, unsigned int>>
        dclInstMap;
    typedef dclInstMap::iterator dclInstMapIter;
    dclInstMap pseudoKills;
    dclInstMap lifetimeEnd;

    unsigned int instId = 0;

    // First populate all pseudo_kills and lifetime.end instructions
    // in BB's inst list. Later run second loop to check whether
    // lifetime rules are flouted.
    for (INST_LIST_ITER it = bb->begin(), end = bb->end(); it != end;
         it++, instId++) {
      G4_INST *curInst = (*it);
      std::pair<G4_INST *, unsigned int> instPair;

      instPair.first = curInst;
      instPair.second = instId;

      if (curInst->isPseudoKill()) {
        pseudoKills.insert(
            make_pair(GetTopDclFromRegRegion(curInst->getDst()), instPair));
      }

      if (curInst->isLifeTimeEnd()) {
        lifetimeEnd.insert(
            make_pair(GetTopDclFromRegRegion(curInst->getSrc(0)), instPair));
      }
    }

    instId = 0;
    for (INST_LIST_ITER it = bb->begin(), end = bb->end(); it != end;
         it++, instId++) {
      G4_INST *curInst = (*it);

      if (curInst->isPseudoKill() || curInst->isLifeTimeEnd()) {
        continue;
      }

      std::stack<G4_Operand *> opnds;
      opnds.push(curInst->getDst());
      opnds.push(curInst->getSrc(0));
      opnds.push(curInst->getSrc(1));
      opnds.push(curInst->getSrc(2));
      opnds.push(curInst->getPredicate());
      opnds.push(curInst->getCondMod());

      while (!opnds.empty()) {
        G4_Operand *curOpnd = opnds.top();
        opnds.pop();

        if (curOpnd != NULL && curOpnd->getTopDcl() != NULL) {
          G4_Declare *topdcl = curOpnd->getTopDcl();

          // Check whether topdcl has been written to map
          dclInstMapIter killsIt = pseudoKills.find(topdcl);

          if (killsIt != pseudoKills.end()) {
            unsigned int foundAtId = (*killsIt).second.second;

            if (foundAtId > instId) {
              DEBUG_VERBOSE("Found a definition before pseudo_kill.");
              (*killsIt).second.first->emit(std::cerr);
              DEBUG_VERBOSE("\n");
              curInst->emit(std::cerr);
              DEBUG_VERBOSE("\n");
            }
          }

          dclInstMapIter lifetimeEndIter = lifetimeEnd.find(topdcl);

          if (lifetimeEndIter != lifetimeEnd.end()) {
            unsigned int foundAtId = (*lifetimeEndIter).second.second;

            if (foundAtId < instId) {
              DEBUG_VERBOSE("Found a use after lifetime.end.");
              (*lifetimeEndIter).second.first->emit(std::cerr);
              DEBUG_VERBOSE("\n");
              curInst->emit(std::cerr);
              DEBUG_VERBOSE("\n");
            }
          }
        }
      }
    }
  }
}

void G4Verifier::verifyOpcode(G4_INST *inst) {
  switch (inst->opcode()) {
  case G4_dp2:
  case G4_dp3:
  case G4_dp4:
    vISA_ASSERT(kernel.fg.builder->hasDotProductInst(), "unsupported opcode");
    break;
  case G4_lrp:
    vISA_ASSERT(kernel.fg.builder->hasLRP(), "unsupported opcode");
    break;
  case G4_madm:
    vISA_ASSERT(kernel.fg.builder->hasMadm(), "unsupported opcode");
    break;
  default:
    break;
  }

  if (passIndex == Optimizer::PI_regAlloc) {
    // ToDo: add more checks for psuedo inst after RA
    vISA_ASSERT(!inst->isPseudoLogic(),
           "pseudo logic inst should be lowered before RA");
  }

  if (inst->getSaturate()) {
    vISA_ASSERT(
        inst->canSupportSaturate(),
        "saturate is set to true but instruction does not support saturation");
  }
}

void G4Verifier::verifyDpas(G4_INST *inst) {
  // Verify region and size of each operands
  G4_InstDpas *dpasInst = inst->asDpasInst();

  if (dpasInst->getPredicate() || dpasInst->getCondMod()) {
    DEBUG_VERBOSE("should not have predicate nor condMod");
    inst->emit(std::cerr);
    DEBUG_VERBOSE("\n");
    vISA_ASSERT(false, "may not have predicate/condMod");
  }
  if (dpasInst->isInt() && (kernel.fg.builder->hasSimplifiedRegions() ||
      kernel.fg.builder->getOption(vISA_GAReArchBugFix))) {
    auto src1Precision = dpasInst->getSrc1Precision();
    auto src2Precision = dpasInst->getSrc2Precision();
    if (dpasInst->GetPrecisionSizeInBits(src1Precision) !=
            dpasInst->GetPrecisionSizeInBits(src2Precision) ||
        src1Precision == GenPrecision::U2 ||
        src1Precision == GenPrecision::S2) {
      VISA_DEBUG_VERBOSE({
        std::cout << "should not have u2/s2 and mixed precision";
        inst->emit(std::cerr);
        std::cout << "\n";
      });
      vISA_ASSERT(false, "u2/s2 and mixed precision is not supported");
    }
  }

  G4_DstRegRegion *dst = dpasInst->getDst();
  G4_Type dTy = dst->getType();
  G4_SrcRegRegion *src0 = dpasInst->getSrc(0)->asSrcRegRegion();
  G4_Type s0Ty = src0->getType();
  G4_SrcRegRegion *src1 = dpasInst->getSrc(1)->asSrcRegRegion();
  G4_Type s1Ty = src1->getType();
  G4_SrcRegRegion *src2 = dpasInst->getSrc(2)->asSrcRegRegion();
  G4_Type s2Ty = src2->getType();
  G4_Operand *opnd3 = dpasInst->getSrc(3);
  G4_SrcRegRegion *src3 = opnd3 ? opnd3->asSrcRegRegion() : nullptr;
  [[maybe_unused]] G4_Type s3Ty = src3 ? src3->getType() : Type_UNDEF;

  // No source modifier
  if (std::any_of(dpasInst->src_begin(), dpasInst->src_end(),
          [](G4_Operand *opnd) {
            return opnd && opnd->asSrcRegRegion()->hasModifier(); })) {
    DEBUG_VERBOSE("should not have source modifier");
    inst->emit(std::cerr);
    DEBUG_VERBOSE("\n");
    vISA_ASSERT(false, "may not have source modifier");
  }

  // No indirect register access
  if (dst->isIndirect() ||
      std::any_of(dpasInst->src_begin(), dpasInst->src_end(),
          [](G4_Operand *opnd) {
            return opnd && opnd->asSrcRegRegion()->isIndirect(); })) {
    DEBUG_VERBOSE("no indirect register access supported!");
    inst->emit(std::cerr);
    DEBUG_VERBOSE("\n");
    vISA_ASSERT(false, "no indirect register access supported!");
  }

  if (dpasInst->opcode() == G4_bdpas) {
    G4_Type s4Ty = dpasInst->getSrc(4)->getType();

    if (!(s1Ty == Type_UD || s1Ty == Type_BF || s1Ty == Type_HF) ||
        !(s2Ty == Type_UD || s2Ty == Type_BF || s2Ty == Type_HF)) {
      DEBUG_VERBOSE("incorrect bdpas type for src1 or src2!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "wrong bdpas type for src1 or src2");
    }
    if (s3Ty != Type_UB || s4Ty != Type_UB) {
      DEBUG_VERBOSE("block scaling src3 and src4 shall be of type UB!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "block scaling src3 and src4 shall be of type UB!");
    }
  } else
  if (!(s1Ty == Type_UD || s1Ty == Type_D) ||
      !(s2Ty == Type_UD || s2Ty == Type_D))
  {
    DEBUG_VERBOSE("incorrect type for src1 or src2!");
    inst->emit(std::cerr);
    DEBUG_VERBOSE("\n");
    vISA_ASSERT(false, "wrong type for src1 or src2");
  }

  if (dpasInst->isInt()) {
    if (!(s0Ty == Type_UD || s0Ty == Type_D) ||
        !(dTy == Type_UD || dTy == Type_D)) {
      DEBUG_VERBOSE("incorrect int type for src0 or dst!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "wrong int type for src0 or dst");
    }
  } else if (dpasInst->isFP16() || dpasInst->isBF16()) {
    G4_Type prec = Type_UNDEF;
    if (dpasInst->getPlatform() >= Xe_PVC) {
      prec = dpasInst->isBF16() ? Type_BF : Type_HF;
    }
    if (!(dTy == Type_F || dTy == prec) || !(s0Ty == Type_F || s0Ty == prec)) {
      DEBUG_VERBOSE("incorrect float type for dst or src0!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "wrong float type for dst or src0");
    }
  } else if (dpasInst->isTF32()) {
    if (dTy != Type_F || s0Ty != Type_F) {
      DEBUG_VERBOSE("incorrect TF32 type for dst or src0 (expecting F)!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "should be float type for dst or src0");
    }
  }

  else if (dpasInst->isFP8()) {
    auto plat = dpasInst->getPlatform();
    if (plat > Xe3) {
      if ((dTy != Type_F && dTy != Type_BF) ||
          (s0Ty != Type_F && s0Ty != Type_BF)) {
        DEBUG_VERBOSE("incorrect type for dst or src0, expecting F or BF!");
        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "should be type F or BF for dst or src0");
      }
    }
    else {
      if (dTy != Type_F || s0Ty != Type_F) {
        DEBUG_VERBOSE("incorrect type for dst or src0, expecting F!");
        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "should be type F for dst or src0");
      }
    }
    if (dpasInst->isHF8() && plat <= Xe3) {
      DEBUG_VERBOSE("incorrect HF8 type for src1 or src2!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "type HF8 not supported for src1 or src2");
    }
  }
  else if (dpasInst->isFP4()) {
    if (!kernel.fg.builder->hasDpasFP4() ||
        !dpasInst->hasSameSrc2Precision(GenPrecision::E2M1)) {
      DEBUG_VERBOSE("incorrect 4-bit float type for src1 or src2!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "4-bit float type not supported for src1 or src2");
    }
    if ((dTy != Type_F && dTy != Type_BF) ||
        (s0Ty != Type_F && s0Ty != Type_BF)) {
      DEBUG_VERBOSE("incorrect type for dst or src0, expecting F or BF!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "should be type F or BF for dst or src0");
    }
  }
  else {
    DEBUG_VERBOSE("invalid!");
    inst->emit(std::cerr);
    DEBUG_VERBOSE("\n");
    vISA_ASSERT(false, "invalid");
  }

  bool invalidRegion = false;
  if (dpasInst->opcode() == G4_bdpas) {
    G4_Operand *opnd4 = dpasInst->getSrc(4);
    G4_SrcRegRegion *src4 = opnd4->asSrcRegRegion();
    invalidRegion = dst->getHorzStride() != 1 ||
                    (!src0->isNullReg() && !src0->getRegion()->isRegion110()) ||
                    !src1->getRegion()->isRegion110() ||
                    !src2->getRegion()->isRegion110() ||
                    (!src3->isNullReg() && !src3->getRegion()->isRegion110()) ||
                    (!src4->isNullReg() && !src4->getRegion()->isRegion110());
  } else
    invalidRegion = dst->getHorzStride() != 1 ||
                    (!src0->isNullReg() && !src0->getRegion()->isRegion110()) ||
                    !src1->getRegion()->isRegion110() ||
                    !src2->getRegion()->isRegion110() ||
                    (src3 && !src3->getRegion()->isRegion110());
  if (invalidRegion) {
    DEBUG_VERBOSE("src region should be <1;1,0> and dst region <1>!");
    inst->emit(std::cerr);
    DEBUG_VERBOSE("\n");
    vISA_ASSERT(false, "src region should be <1;1,0> and dst region <1>!");
  }

  // register alignment & size
  //   dst & src0 : aligned on execsize
  //   src1 : aligned on grf
  //   src2 : aligned on systolic depth * OPS_PER_CHAN
  if (passIndex == Optimizer::PI_regAlloc) {
    uint32_t D = dpasInst->getSystolicDepth();
    uint32_t ES = dpasInst->getExecSize();
    uint32_t RC = dpasInst->getRepeatCount();
    uint32_t Src1_D = D;

    if (RC > 8) {
      DEBUG_VERBOSE("repeat count must be 1 to 8!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "repeat count must be 1 to 8!");
    }

    else if (dpasInst->opcode() == G4_bdpas) {
      if (ES != 16) {
        DEBUG_VERBOSE("execsize must be 16!");
        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "execsize must be 16!");
      }

      if (RC != 8) {
        DEBUG_VERBOSE("repeat count must be 8!");
        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "repeat count must be 8!");
      }

      if (D != 8) {
        DEBUG_VERBOSE("depth must be 8!");
        inst->emit(std::cerr);
        DEBUG_VERBOSE("\n");
        vISA_ASSERT(false, "depth must be 8!");
      }

      G4_SrcRegRegion *src4 = dpasInst->getSrc(4)->asSrcRegRegion();
      if (dpasInst->isFP16() || dpasInst->isBF16() || dpasInst->isFP8()) {
        if ((src3->getSubRegOff() & 15) != 0) {
          DEBUG_VERBOSE("For 16-bit and 8-bit data types, src3 subregisters must "
                        "be {0, 16, 32, 48}:ub");
          inst->emit(std::cerr);
          DEBUG_VERBOSE("\n");
          vISA_ASSERT(false, "invalid subreg for src3");
        }
        if ((src4->getSubRegOff() & 7) != 0) {
          DEBUG_VERBOSE("For 16-bit and 8-bit data types, src4 subregisters must "
                        "be {0, 8, 16, 24, 32, 40, 48, 56}:ub");
          inst->emit(std::cerr);
          DEBUG_VERBOSE("\n");
          vISA_ASSERT(false, "invalid subreg for src4");
        }
      } else if (dpasInst->isFP4()) {
        if (src3->getSubRegOff() > 16 || (src3->getSubRegOff() & 15) != 0) {
          DEBUG_VERBOSE("For 4-bit data types, src3 subregisters must be "
                        "{0, 16}:ub");
          inst->emit(std::cerr);
          DEBUG_VERBOSE("\n");
          vISA_ASSERT(false, "invalid subreg for src3");
        }
        if (src4->getSubRegOff() > 24 || (src4->getSubRegOff() & 7) != 0) {
          DEBUG_VERBOSE("For 4-bit data types, src4 subregisters must "
                        "be {0, 8, 16, 24}:ub");
          inst->emit(std::cerr);
          DEBUG_VERBOSE("\n");
          vISA_ASSERT(false, "invalid subreg for src4");
        }
      }
    }

    uint32_t dAlignBytes = TypeSize(dTy) * ES;
    uint32_t s0AlignBytes = TypeSize(s0Ty) * ES;
    if ((dst->getLinearizedStart() % dAlignBytes) != 0 ||
        (src0->getLinearizedStart() % s0AlignBytes) != 0) {
      DEBUG_VERBOSE(
        "dst/src0's subreg offset should be multiple of execsize!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false,
        "dst/src0's subreg offset should be multiple of execsize!");
    }

    uint32_t dBytes = dst->getLinearizedEnd() - dst->getLinearizedStart() + 1;
    uint32_t s0Bytes =
        src0->getLinearizedEnd() - src0->getLinearizedStart() + 1;
    if (dBytes != (dAlignBytes * RC) ||
        (!src0->isNullReg() && s0Bytes != s0AlignBytes * RC)) {
      DEBUG_VERBOSE("dst/src0's size is wrong!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "dst/src0's size is wrong!");
    }

    if ((src1->getLinearizedStart() % kernel.numEltPerGRF<Type_UB>()) != 0) {
      DEBUG_VERBOSE("src1's subreg offset should be 0!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "src1's subreg offset should be 0!");
    }

    // bytes per lane per depth
    uint32_t bytes1PerLD = dpasInst->getSrc1SizePerLaneInByte();
    uint32_t s1ExpectedBytes = bytes1PerLD * Src1_D * ES;
    uint32_t s1Bytes =
      src1->getLinearizedEnd() - src1->getLinearizedStart() + 1;
    if (s1Bytes != s1ExpectedBytes) {
      DEBUG_VERBOSE("src1's size is wrong!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "src1's size is wrong!");
    }

    uint32_t s2AlignBytes = dpasInst->getSrc2SizePerLaneInByte() * D;
    if ((src2->getLinearizedStart() % s2AlignBytes) != 0) {
      DEBUG_VERBOSE("src2's subreg offset is incorrec!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "src2's subreg offset is incorrect!");
    }

    uint32_t s2Bytes =
        src2->getLinearizedEnd() - src2->getLinearizedStart() + 1;
    uint32_t correctBytes = s2AlignBytes * RC;
    if (dpasInst->opcode() == G4_dpasw) {
      correctBytes = s2AlignBytes * ((RC + 1) / 2);
    }
    if (s2Bytes != correctBytes) {
      DEBUG_VERBOSE("src2's size is wrong!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "src2's size is wrong!");
    }
  }
}

void G4Verifier::verifyAccMov(G4_INST *inst) {
  const G4_Operand *src = inst->getSrc(0);
  const G4_Operand *dst = inst->getDst();
  if (kernel.fg.builder->hasFormatConversionACCRestrictions() &&
      inst->opcode() == G4_mov && (src->isAccReg() || dst->isAccReg())) {
    const bool allowedICombination =
        (IS_DTYPE(src->getType()) || src->getType() == Type_W ||
         src->getType() == Type_UW) &&
        (IS_DTYPE(dst->getType()) || dst->getType() == Type_W ||
         dst->getType() == Type_UW);
    const bool allowedFCombination =
        (src->getType() == Type_F || src->getType() == Type_HF) &&
        (dst->getType() == Type_F || dst->getType() == Type_HF);
    const bool allowedDFCombination =
        src->getType() == Type_DF && dst->getType() == Type_DF;
    if (!allowedICombination && !allowedFCombination && !allowedDFCombination) {
      DEBUG_VERBOSE("Invalid type combination during mov format conversion "
                    "when accumulator is used as src or dst!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "Invalid type combination during mov format "
                          "conversion when accumulator is used as src or dst!");
    }
  }
}

//
// Mixed mode instruction allows bfloat16 operands in the following cases:
//   1. dst, src0, and src1 for 2 source instructions format not involving
//   multiplier(mov, add, cmp, sel).
//   2. dst and src0 for 2 source instructions format involving multiplier(mul,
//   mac etc).
//   3. dst, src0, and src1 for 3 source instructions format(mad).
//   4. Broadcast of bfloat16 scalar is not supported.
//   5. Unpacked bfloat16 destination with stride 2 when register offset is 0
//   or 1.
//   6. Packed bfloat16 source and destination when register offset is 0 or 8.
//   7. Execution size must not be greater than 8.
//   8. Instructions with pure bfloat16 operands are not supported.
//
// **More examples**
//   1. BF imm is not allowed
//      mov  (1|M0)  r12.0<1>:f  0xffff:bf - ILLEGAL "Imm operand with BF type
//      is not allowed"
//   2. BF scalar operand can be used in SIMD1
//      mul  (1|M0)  r14.0<1>:f  r11.0<0;1,0>:bf  r12.3<0;1,0>:f - OK
//   3. For SIMD1, scalar operands (both dst/src) of F or BF can have any
//   subreg!
//      add  (1|M0)  r16.3<1>:bf  r11.0<0;1,0>:f  r12.3<0;1,0>:f - OK
//   4. F Operand should have subreg = 0 if execSize > SIMD1
//      add  (2|M0)  r10.4<1>:f  r11.0<1;1,0>:bf   0x12345:f
//       ILLEGAL "Src0 regioning must be aligned to destination or scalar for
//       Float/64bit pipes"
//   5. Others
//     add  (8|M0)  r16.0<2>:bf  r11.0<1;1,0>:f  r12.0<1;1,0>:f- OK
//     add  (8|M0)  r16.1<2>:bf  r11.0<1;1,0>:f  r12.8<1;1,0>:f- OK
//     add  (8|M0)  r16.0<1>:bf  r11.0<1;1,0>:f  r12.8<1;1,0>:f- OK
//     add  (8|M0)  r16.8<1>:bf  r11.0<1;1,0>:f  r12.0<1;1,0>:f- OK
//         Note that float source operands  can be scalar region <0;1,0>
//
//   For PVC, case 6 should be "Execution size must not be greater than 16."
void G4Verifier::verifyBFMixedMode(G4_INST *inst) {
  auto useGivenType = [](G4_INST *I, G4_Type GivenTy) -> bool {
    G4_Operand *dst = I->getDst();
    if (I->isPseudoAddrMovIntrinsic()) {
      return false;
    }
    // Skip compare's dst (?)
    if (dst && !dst->isNullReg() && !I->isCompare()) {
      if (dst->getType() == GivenTy)
        return true;
    }
    for (int i = 0; i < I->getNumSrc(); ++i) {
      G4_Operand *src = I->getSrc(i);
      if (src && !src->isNullReg()) {
        if (src->getType() == GivenTy)
          return true;
      }
    }
    return false;
  };

  // Skip dpas/send as it has been verified separately
  if (inst->isDpas() || inst->isSend())
    return;

  // Skip if no BF usage
  if (!useGivenType(inst, Type_BF))
    return;

  if (!kernel.fg.builder->hasBFMixMode()) {
    DEBUG_VERBOSE("BF type: BF mixed mode not supported!");
    inst->emit(std::cerr);
    DEBUG_VERBOSE("\n");
    vISA_ASSERT(false, "BF type: BF mixed mode not supported!!");
  }
  if (inst->isPureBFInst() && kernel.fg.builder->supportPureBF()) {
    verifyPureBFMode(inst);
    return;
  }

  if (inst->getPlatform() > Xe3) {
    // Except for mov, fcvt and srnd instructions, HWConformityPro has fixed
    // the BF mixed mode and BF pure mode by promoting all BF operands to float.
    vISA_ASSERT(inst->opcode() == G4_mov || inst->opcode() == G4_fcvt ||
                    inst->opcode() == G4_srnd,
                "BF mixed or pure modes are not allowed by vISA!!");
    return;
  }

  // case 8, pure bf not supported
  if (!useGivenType(inst, Type_F)) {
    DEBUG_VERBOSE("Pure BF operands are not supported!");
    inst->emit(std::cerr);
    DEBUG_VERBOSE("\n");
    vISA_ASSERT(false, "Pure BF operands are not supported!!");
  }

  switch (inst->opcode()) {
  case G4_mul:
  case G4_mac: {
    // case 2
    G4_Operand *src1 = inst->getSrc(1);
    if (src1->getType() != Type_F) {
      DEBUG_VERBOSE("Src1 in BF mixed mode must be F!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "Src1 in BF mixed mode must be F!");
    }
    break;
  }
  case G4_mad:
  case G4_pseudo_mad: {
    // case 3
    //    Note that G4_pseudo_mad : src0*src1 + src2
    //                    gen mad : src0 + src1*src2
    //    Need to switch 0 with 2 for pseudo_mad
    G4_Operand *src2 = inst->getSrc(inst->opcode() == G4_pseudo_mad ? 0 : 2);
    if (src2->getType() != Type_F) {
      DEBUG_VERBOSE("Src2 in BF mixed mode must be F!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "Src2 in BF mixed mode must be F!");
    }
    break;
  }
  case G4_mov: {
    if (inst->getSrc(0)->getType() == Type_BF) {
      // bf->f is just a left shift, bf mix restriction does not apply.
      return;
    }
    // case 1
    break;
  }
  case G4_add:
  case G4_sel:
  case G4_cmp: { // case 1
    break;
  }
  default:
    DEBUG_VERBOSE("Instruction does not support BF type!");
    inst->emit(std::cerr);
    DEBUG_VERBOSE("\n");
    vISA_ASSERT_UNREACHABLE("Instruction does not support BF type!");
    break;
  }

  uint32_t nativeES = kernel.fg.builder->getNativeExecSize();
  // verify dst
  G4_DstRegRegion *dreg = inst->getDst();
  if (dreg && !dreg->isNullReg() && !inst->isCompare()) {
    uint32_t hs = dreg->getHorzStride();
    uint32_t so = dreg->getSubRegOff();
    bool isLegitPackedBF = (dreg->getType() == Type_BF &&
                            (hs == 1 && (so == 0 || so == nativeES)));
    bool isLegitUnpackedBF =
        (dreg->getType() == Type_BF && (hs == 2 && (so == 0 || so == 1)));
    bool isLegitF = (dreg->getType() == Type_F && (hs == 1 && so == 0));
    bool isLegitScalar = (inst->getExecSize() == g4::SIMD1 && hs == 1);
    if (!(isLegitPackedBF || isLegitUnpackedBF || isLegitF || isLegitScalar)) {
      // case 5 & 6
      DEBUG_VERBOSE("BF/F Dst has illegal region and type combination!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "BF/F Dst has illegal region and type combination!");
    }
  }

  // verify src
  for (int i = 0, sz = (int)inst->getNumSrc(); i < sz; ++i) {
    G4_Operand *src = inst->getSrc(i);
    if (!src || src->isNullReg() // sanity
        || (src->getType() == Type_F && src->isImm()))
      continue;

    G4_Type srcTy = src->getType();
    if (srcTy == Type_BF &&
        (src->isImm() || (inst->getExecSize() != g4::SIMD1 &&
                          src->asSrcRegRegion()->getRegion()->isScalar()))) {
      // case 4
      DEBUG_VERBOSE(" Src: Imm BF/broadcast scalar BF are not supported!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "Src: Imm BF/broadcast scalar BF are not supported!");
    }

    G4_SrcRegRegion *sreg = src->asSrcRegRegion();
    uint32_t so = sreg->getSubRegOff();
    bool isLegitPackedBF =
        (srcTy == Type_BF && !sreg->getRegion()->isScalar() &&
         sreg->getRegion()->isContiguous(inst->getExecSize()) &&
         (so == 0 || so == nativeES));
    bool isLegitF =
        (srcTy == Type_F && !sreg->getRegion()->isScalar() &&
         sreg->getRegion()->isContiguous(inst->getExecSize()) && so == 0);
    bool isLegitScalar =
        (sreg->getRegion()->isScalar() &&
         (srcTy == Type_F ||
          (srcTy == Type_BF && inst->getExecSize() == g4::SIMD1)));
    if (!(isLegitPackedBF || isLegitF || isLegitScalar)) {
      // case 5 & 6
      DEBUG_VERBOSE("Src has illegal region and type combination!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false, "Src has illegal region and type combination!");
    }
  }

  // case 7
  if (inst->getExecSize() > nativeES) {
    std::stringstream ss;
    ss << "Inst in BF mixed mode should have execsize <= " << nativeES << '\n';
    DEBUG_VERBOSE(ss.str().c_str());
    inst->emit(std::cerr);
    DEBUG_VERBOSE("\n");
    vISA_ASSERT(false, ss.str().c_str());
  }
  return;
}
void G4Verifier::verifyPureBFMode(G4_INST *inst) {
  if (!inst->isPureBFInst())
    return;

  if (!inst->canSupportPureBF()) {
    DEBUG_VERBOSE("Instruction does not support pure BF mode!");
    inst->emit(std::cerr);
    DEBUG_VERBOSE("\n");
    vISA_ASSERT_UNREACHABLE("Instruction does not support pure BF mode!");
  }

  auto dst = inst->getDst();
  uint32_t dstSubRegOffset = 0;
  bool dstHasFixedSubregOffset = false;
  uint32_t dstStrideInBytes = dst->getExecTypeSize();
  if (dst->isNullReg()) {
    dstSubRegOffset = 0;
    dstHasFixedSubregOffset = true;
  } else {
    dstHasFixedSubregOffset =
        dst->hasFixedSubregOffset(inst->getBuilder(), dstSubRegOffset);
  }

  for (int i = 0, sz = (int)inst->getNumSrc(); i < sz; ++i) {
    auto src = inst->getSrc(i);
    if (src->isImm()) {
      DEBUG_VERBOSE(
          "Src can not be immediate operand in pure BF mode!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(false,
          "Dst can not be immediate operand in pure BF mode!");
    }

    auto srcRR = src->asSrcRegRegion();
    if (srcRR->isScalar())
      continue;

    uint32_t srcSubRegOffset = 0;
    bool srcHasFixedSubregOffset = false;
    uint16_t srcStride = 0;
    srcRR->getRegion()->isSingleStride(inst->getExecSize(), srcStride);
    uint32_t srcStrideInBytes = srcStride * src->getTypeSize();
    if (src->isNullReg()) {
      srcSubRegOffset = 0;
      srcHasFixedSubregOffset = true;
    } else {
      srcHasFixedSubregOffset =
          srcRR->hasFixedSubregOffset(inst->getBuilder(), srcSubRegOffset);
    }

    bool hasValidRegion = dstHasFixedSubregOffset && srcHasFixedSubregOffset &&
                          (dstSubRegOffset == srcSubRegOffset) &&
                          (dstStrideInBytes == srcStrideInBytes);
    if (!hasValidRegion && inst->isMath()) {
      hasValidRegion = dstHasFixedSubregOffset && srcHasFixedSubregOffset &&
                       (dstStrideInBytes == srcStrideInBytes) &&
                       (srcSubRegOffset / 4 == dstSubRegOffset / 4);
    }
    if (!hasValidRegion) {
      DEBUG_VERBOSE(
          "register regioning is illegal for this pure BF instruction!");
      inst->emit(std::cerr);
      DEBUG_VERBOSE("\n");
      vISA_ASSERT(
          false, "register regioning is illegal for this pure BF instruction!");
    }
  }

  return;
}

void G4Verifier::verifyLfsr(G4_INST *inst) {
  const G4_InstLfsr *lfsrInst = inst->asLfsrInst();

  // No saturation
  if (inst->getSaturate())
    vISA_ASSERT(false, "lfsr doesn't support saturation");

  G4_DstRegRegion *dst = lfsrInst->getDst();
  [[maybe_unused]] G4_Type dTy = dst->getType();
  vISA_ASSERT(dTy == Type_UD, "dst type of lfsr must be UD");
  vISA_ASSERT(dst->getHorzStride() == 1, "dst region of lfsr must be <1>");

  [[maybe_unused]] G4_SrcRegRegion *src0 =
      lfsrInst->getSrc(0)->asSrcRegRegion();
  [[maybe_unused]] G4_Type s0Ty = src0->getType();
  vISA_ASSERT(s0Ty == Type_UD, "src0 type of lfsr must be UD");
  vISA_ASSERT(!src0->hasModifier(), "lfsr doesn't support source modifier");
  vISA_ASSERT(src0->getRegion()->isRegion110() || src0->getRegion()->isScalar(),
              "src0 region of lfsr must be <1;1,0> or <0;1,1>");
  vISA_ASSERT(!src0->isIndirect(), "lfsr doesn't support indirect access");

  if (lfsrInst->getSrc(1)->isSrcRegRegion()) {
    [[maybe_unused]] G4_SrcRegRegion *src1 =
        lfsrInst->getSrc(1)->asSrcRegRegion();
    [[maybe_unused]] G4_Type s1Ty = src1->getType();
    vISA_ASSERT(s1Ty == Type_UD, "src1 type of lfsr must be UD");
    vISA_ASSERT(!src1->hasModifier(), "lfsr doesn't support source modifier");
    vISA_ASSERT(src1->getRegion()->isRegion110() ||
                    src1->getRegion()->isScalar(),
                "src1 region of lfsr must be <1;1,0> or <0;1,0>");
    vISA_ASSERT(!src1->isIndirect(), "lfsr doesn't support indirect access");
  }
}

void G4Verifier::verifyDnscl(G4_INST *inst) {
  const G4_InstDnscl *dnsclInst = inst->asDnsclInst();

  G4_DstRegRegion *dst = dnsclInst->getDst();
  G4_Type dTy = dst->getType();
  G4_SrcRegRegion *src0 = dnsclInst->getSrc(0)->asSrcRegRegion();
  G4_Type s0Ty = src0->getType();
  G4_SrcRegRegion *src1 = dnsclInst->getSrc(1)->asSrcRegRegion();
  G4_Type s1Ty = src1->getType();
  G4_SrcRegRegion *src2 = dnsclInst->getSrc(2)->asSrcRegRegion();
  G4_Type s2Ty = src2->getType();

  // No saturation
  if (inst->getSaturate())
    vISA_ASSERT(false, "dnscl doesn't support saturation");

  // No source modifier
  if (src0->hasModifier() || src1->hasModifier() || src2->hasModifier())
    vISA_ASSERT(false, "dnscl doesn't support source modifier");

  // No indirect register access
  if (src0->isIndirect() || src1->isIndirect() || src2->isIndirect())
    vISA_ASSERT(false, "dnscl doesn't support indirect access");

  // All dst/src operands must be UD datatype
  if (dTy != Type_UD || s0Ty != Type_UD || s1Ty != Type_UD || s2Ty != Type_UD)
    vISA_ASSERT(false, "only UD data type is allowed for dnscl");

  // Dst region must be <1>
  if (dst->getHorzStride() != 1)
    vISA_ASSERT(false, "dst region of dnscl must be <1>");

  // Src0/src1 region must be <1;1,0>, src2 region must be <1;1,0>, or <2;2,1>
  // due to normalizeRegion pass.
  if (inst->getExecSize() != g4::SIMD1) {
    if (!src0->getRegion()->isRegion110() || !src1->getRegion()->isRegion110())
      vISA_ASSERT(false, "src0/src1 region of dnscl must be <1;1,0>");

    bool src2IsRegion221 = src2->getRegion()->vertStride == 2 &&
                           src2->getRegion()->width == 2 &&
                           src2->getRegion()->horzStride == 1;
    if (!src2->isNullReg() &&
        !(src2->getRegion()->isRegion110() || src2IsRegion221))
      vISA_ASSERT(false, "src2 region of dnscl must be <1;1,0> or <2;2,1>");
  }
}
