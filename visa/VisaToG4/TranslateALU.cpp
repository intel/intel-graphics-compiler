/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../Timer.h"
#include "BuildIR.h"

using namespace vISA;

int IR_Builder::translateVISAAddrInst(
    ISA_Opcode opcode, VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
    G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd, G4_Operand *src1Opnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize exsize = toExecSize(executionSize);
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, exsize);

  if (src1Opnd && src0Opnd->isAddrExp() && src1Opnd->isImm()) {
    src0Opnd->asAddrExp()->setOffset(src0Opnd->asAddrExp()->getOffset() +
                                     (int)src1Opnd->asImm()->getInt());
    src1Opnd = NULL;
  }

  if (src0Opnd->isAddrExp() && src1Opnd == NULL) {
    createMov(exsize, dstOpnd, src0Opnd, instOpt, true);
  } else {
    createInst(NULL, GetGenOpcodeFromVISAOpcode((ISA_Opcode)opcode), NULL,
               g4::NOSAT, exsize, dstOpnd, src0Opnd, src1Opnd, instOpt, true);
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISAArithmeticInst(
    ISA_Opcode opcode, VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
    G4_Predicate *predOpnd, G4_Sat saturate, G4_CondMod *condMod,
    G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd, G4_Operand *src1Opnd,
    G4_Operand *src2Opnd, G4_DstRegRegion *carryBorrow) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  unsigned int instOpt = 0;
  G4_ExecSize exsize = toExecSize(executionSize);
  instOpt |= Get_Gen4_Emask(emask, exsize);

  if (IsMathInst(opcode)) {
    if (src1Opnd == NULL) {
      // create a null operand
      src1Opnd = createNullSrc(src0Opnd->getType());
    }

    G4_MathOp mathOp = Get_MathFuncCtrl(opcode, dstOpnd->getType());

    if (!hasFdivPow() && mathOp == MATH_FDIV) {
      expandFdiv(exsize, predOpnd, saturate, dstOpnd, src0Opnd, src1Opnd,
                 instOpt);
    } else if (!hasFdivPow() && mathOp == MATH_POW) {
      expandPow(exsize, predOpnd, saturate, dstOpnd, src0Opnd, src1Opnd,
                instOpt);
    } else {
      createMathInst(predOpnd, saturate, exsize, dstOpnd, src0Opnd, src1Opnd,
                     mathOp, instOpt, true);
    }
  } else if (ISA_Inst_Table[opcode].n_srcs == 3) {
    if (opcode == ISA_ADD3O) {
      vISA_ASSERT_INPUT(predOpnd != nullptr, "predicate operand couldn't be nullptr");
      condMod = createCondMod(Mod_o, predOpnd->getBase(), 0);
      predOpnd = nullptr;
    }

    if (opcode == ISA_MADW && !supportNativeSIMD32()) {
      vISA_ASSERT_INPUT(exsize <= 16,
                        "Unsupported execution size for madw");
    }

    // do not check type of sources, float and integer are supported
    auto inst = createInst(predOpnd, GetGenOpcodeFromVISAOpcode(opcode),
                           condMod, saturate, exsize, dstOpnd, src0Opnd,
                           src1Opnd, src2Opnd, instOpt, true);

    if (opcode == ISA_MADW) {
      G4_DstRegRegion *accDstOpnd =
          createDst(phyregpool.getAcc0Reg(), 0, 0, 1, dstOpnd->getType());

      inst->setImplAccDst(accDstOpnd);
      inst->setOptionOn(InstOpt_AccWrCtrl); // to be consistent with impl Acc.
    }
  } else {
    auto inst = createInst(predOpnd, GetGenOpcodeFromVISAOpcode(opcode),
                           condMod, saturate, exsize, dstOpnd, src0Opnd,
                           src1Opnd, instOpt, true);

    if (opcode == ISA_ADDC || opcode == ISA_SUBB) {
      G4_DstRegRegion *accDstOpnd =
          createDst(phyregpool.getAcc0Reg(), 0, 0, 1, dstOpnd->getType());

      inst->setImplAccDst(accDstOpnd);
      inst->setOptionOn(InstOpt_AccWrCtrl);

      // mov dst acc0
      G4_SrcRegRegion *accSrcOpnd =
          createSrc(phyregpool.getAcc0Reg(), 0, 0, getRegionStride1(),
                    dstOpnd->getType());

      createMov(exsize, carryBorrow, accSrcOpnd, instOpt, true);
    } else if (opcode == ISA_PLANE) {
      const RegionDesc *rd = createRegionDesc(0, 4, 1);
      auto src0 = inst->getSrc(0);
      vISA_ASSERT(src0->isSrcRegRegion(),
                  " Src0 of plane inst is not regregion.");
      src0->asSrcRegRegion()->setRegion(*this, rd);
    }
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISADpasInst(VISA_Exec_Size executionSize,
                                      VISA_EMask_Ctrl emask, G4_opcode opc,
                                      G4_DstRegRegion *dstOpnd,
                                      G4_SrcRegRegion *src0Opnd,
                                      G4_SrcRegRegion *src1Opnd,
                                      G4_SrcRegRegion *src2Opnd,
                                      G4_SrcRegRegion *src3Opnd,
                                      G4_SrcRegRegion *src4Opnd, GenPrecision A,
                                      GenPrecision W, uint8_t D, uint8_t C) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize exsize = toExecSize(executionSize);
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, exsize);
  if (hasBFDstforDPAS() && A == GenPrecision::BF16) {
    // PVC allows BF dst and src0, and they are W/UW when coming into vISA,
    // so we fix the type here
    // 8/22: Since visa introduced BF type, using W/UW for BF is no longer
    // necessary.
    //       After visa's inputs all use BF directly, this code should be
    //       deleted.
    if (dstOpnd->getType() == Type_W || dstOpnd->getType() == Type_UW) {
      dstOpnd->setType(*this, Type_BF);
    }
    if (src0Opnd->getType() == Type_W || src0Opnd->getType() == Type_UW) {
      src0Opnd->setType(*this, Type_BF);
    }
  }

  if (src0Opnd->isNullReg()) {
    src0Opnd->setType(*this, dstOpnd->getType());
  }

  createDpasInst(opc, exsize, dstOpnd, src0Opnd, src1Opnd, src2Opnd, src3Opnd,
                 src4Opnd, instOpt, A, W, D, C, true);

  return VISA_SUCCESS;
}

int IR_Builder::translateVISABfnInst(
    uint8_t booleanFuncCtrl, VISA_Exec_Size executionSize,
    VISA_EMask_Ctrl emask, G4_Predicate *predOpnd, G4_Sat saturate,
    G4_CondMod *condMod, G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd,
    G4_Operand *src1Opnd, G4_Operand *src2Opnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  unsigned int instOpt = 0;
  G4_ExecSize exsize = toExecSize(executionSize);
  instOpt |= Get_Gen4_Emask(emask, exsize);

  createBfnInst(booleanFuncCtrl, predOpnd, condMod, saturate, exsize, dstOpnd,
                src0Opnd, src1Opnd, src2Opnd, instOpt, true);

  return VISA_SUCCESS;
}

static bool needs32BitFlag(uint32_t opt) {
  switch (opt & InstOpt_QuarterMasks) {
  case InstOpt_M16:
  case InstOpt_M20:
  case InstOpt_M24:
  case InstOpt_M28:
    return true;
  default:
    return false;
  }
}

int IR_Builder::translateVISACompareInst(
    ISA_Opcode opcode, VISA_Exec_Size execsize, VISA_EMask_Ctrl emask,
    VISA_Cond_Mod relOp, G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd,
    G4_Operand *src1Opnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_CondMod *condMod = NULL;
  G4_ExecSize exsize = toExecSize(execsize);
  G4_InstOpts inst_opt = Get_Gen4_Emask(emask, exsize);
  const char *varName = getNameString(50, "PTemp_%" PRIu64, kernel.Declares.size());

  uint8_t numWords = (exsize + 15) / 16;
  if (needs32BitFlag(inst_opt)) {
    // for H2, Q3, etc. we must use 32-bit flag regardless of execution size
    numWords = 2;
  }
  // TODO: Can eliminate the flag temp creation. Might need further changes
  G4_Declare *dcl =
      createDeclare(varName, G4_FLAG, numWords, 1, Type_UW);

  condMod =
      createCondMod(Get_G4_CondModifier_From_Common_ISA_CondModifier(relOp),
                    dcl->getRegVar(), 0);

  createInst(nullptr, GetGenOpcodeFromVISAOpcode(opcode), condMod, g4::NOSAT,
             exsize, dstOpnd, src0Opnd, src1Opnd, inst_opt, true);

  return VISA_SUCCESS;
}

int IR_Builder::translateVISACompareInst(
    ISA_Opcode opcode, VISA_Exec_Size execsize, VISA_EMask_Ctrl emask,
    VISA_Cond_Mod relOp, G4_Declare *predDst, G4_Operand *src0Opnd,
    G4_Operand *src1Opnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize exsize = toExecSize(execsize);
  G4_InstOpts inst_opt = Get_Gen4_Emask(emask, exsize);
  // If it's mix mode HF,F, it will be split down the road anyway, so behavior
  // doesn't change.
  G4_Type src0Type = src0Opnd->getType();
  G4_Type src1Type = src1Opnd->getType();
  G4_Type dstType;
  if (IS_TYPE_FLOAT_ALL(src0Type)) {
    dstType = (TypeSize(src0Type) > TypeSize(src1Type)) ? src0Type : src1Type;
  } else {
    // FIXME: why does exec size matter here?
    dstType =
        exsize == 16
            ? Type_W
            : (TypeSize(src0Type) > TypeSize(src1Type) ? src0Type : src1Type);
    if (IS_VTYPE(dstType)) {
      dstType = Type_UD;
    }
  }
  auto nullDst = createNullDst(dstType);

  G4_CondMod *condMod =
      createCondMod(Get_G4_CondModifier_From_Common_ISA_CondModifier(relOp),
                    predDst->getRegVar(), 0);

  createInst(NULL, GetGenOpcodeFromVISAOpcode(opcode), condMod, g4::NOSAT,
             exsize, nullDst, src0Opnd, src1Opnd, inst_opt, true);

  return VISA_SUCCESS;
}

int IR_Builder::translateVISALogicInst(
    ISA_Opcode opcode, G4_Predicate *predOpnd, G4_Sat saturate,
    VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask, G4_DstRegRegion *dst,
    G4_Operand *src0, G4_Operand *src1, G4_Operand *src2, G4_Operand *src3) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize exsize = toExecSize(executionSize);
  G4_InstOpts inst_opt = Get_Gen4_Emask(emask, exsize);
  G4_Operand *g4Srcs[] = {src0, src1, src2, src3};

  G4_opcode g4_op = GetGenOpcodeFromVISAOpcode(opcode);
  if (dst->getBase() && dst->getBase()->isFlag()) {
    g4_op = Get_Pseudo_Opcode(opcode);
    if (g4_op == G4_illegal) {
      return VISA_FAILURE;
    }
  }

  for (int i = 0; i < ISA_Inst_Table[opcode].n_srcs; i++) {
    if (g4Srcs[i]->isSrcRegRegion() && !isShiftOp(opcode) &&
        (g4Srcs[i]->asSrcRegRegion()->getModifier() == Mod_Minus ||
         g4Srcs[i]->asSrcRegRegion()->getModifier() == Mod_Minus_Abs)) {
      G4_Type tmpType = g4Srcs[i]->asSrcRegRegion()->getType();
      G4_Declare *tempDcl = createTempVar(exsize, tmpType, Any);
      G4_DstRegRegion *tmp_dst_opnd =
          createDst(tempDcl->getRegVar(), 0, 0, 1, tmpType);

      uint16_t vs = exsize;
      if ((unsigned)exsize * g4Srcs[i]->asSrcRegRegion()->getTypeSize() >
          numEltPerGRF<Type_UB>()) {
        vs /= 2;
      }

      createMov(exsize, tmp_dst_opnd, g4Srcs[i], inst_opt, true);

      g4Srcs[i] = createSrcRegRegion(tempDcl, getRegionStride1());
    }
  }

  if (opcode == ISA_BFI || opcode == ISA_BFE || opcode == ISA_BFREV) {
    // convert all immediates to D or UD as required by HW
    // ToDo: maybe we should move this to HW conformity?
    for (int i = 0; i < 4; i++) {
      if (g4Srcs[i] != NULL && g4Srcs[i]->isImm()) {
        G4_Imm *imm = g4Srcs[i]->asImm();
        switch (imm->getType()) {
        case Type_W:
          g4Srcs[i] = createImm(imm->getInt(), Type_D);
          break;
        case Type_UW:
          g4Srcs[i] = createImm(imm->getInt(), Type_UD);
          break;
        default:
          // ignore other types to be consistent with old behavior
          break;
        }
      }
    }
  }

  if (opcode == ISA_BFI) {
    // split into
    // bfi1 tmp src0 src1
    // bfi2 dst tmp src2 src3
    G4_Declare *tmpDcl =
        createTempVar(exsize, g4Srcs[0]->getType(), getGRFAlign());
    G4_DstRegRegion *tmpDst = createDstRegRegion(tmpDcl, 1);
    createInst(duplicateOperand(predOpnd), g4_op, nullptr, saturate,
               exsize, // it is number of bits for predicate logic op
               tmpDst, g4Srcs[0], g4Srcs[1], inst_opt, true);

    G4_SrcRegRegion *src0 = createSrcRegRegion(
        tmpDcl, (exsize == 1) ? getRegionScalar() : getRegionStride1());
    createInst(predOpnd, G4_bfi2, nullptr, saturate,
               exsize, // it is number of bits for predicate logic op
               dst, src0, g4Srcs[2], g4Srcs[3], inst_opt, true);
  } else {
    // create inst
    createInst(predOpnd, g4_op, nullptr, saturate,
               exsize, // it is number of bits for predicate logic op
               dst, g4Srcs[0], g4Srcs[1], g4Srcs[2], inst_opt, true);
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISADataMovementInst(
    ISA_Opcode opcode, CISA_MIN_MAX_SUB_OPCODE subOpcode,
    G4_Predicate *predOpnd, VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
    G4_Sat saturate, G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd,
    G4_Operand *src1Opnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize exsize = toExecSize(executionSize);
  G4_InstOpts inst_opt = Get_Gen4_Emask(emask, exsize);
  G4_CondMod *condMod = NULL;

  if (opcode == ISA_MOVS) {
    if (src0Opnd->isSrcRegRegion())
      src0Opnd->asSrcRegRegion()->setType(*this, Type_UD);
    dstOpnd->setType(*this, Type_UD);
    vISA_ASSERT_INPUT(saturate == g4::NOSAT,
                 "saturation forbidden on this instruction");
    createInst(predOpnd, G4_mov, NULL, g4::NOSAT, exsize, dstOpnd, src0Opnd,
               NULL, inst_opt, true);
  } else if (opcode == ISA_SETP) {
    // Src0 must have integer type.  If src0 is a general or indirect operand,
    // the LSB in each src0 element determines the corresponding dst element's
    // Bool value. If src0 is an immediate operand, each of its bits from the
    // LSB to MSB is used to set the Bool value in the corresponding dst
    // element. Predication is not supported for this instruction.

    /*
     * 1. Mask operand is const or scalar
     *   mov (1) f0.0 src {NoMask}
     * 2. Mask operand is stream.
     *   and.nz.f0.0 (n) null src 0x1:uw
     */

    // vISA spec does not allow 1 as the execution size anymore.
    // This is a hack to allow execution size 1
    // and we make sure it is a scalar region in this case.
    if (kernel.getKernelType() == VISA_CM) {
      if (exsize == 1 && src0Opnd->isSrcRegRegion()) {
        G4_SrcRegRegion *region = src0Opnd->asSrcRegRegion();
        if (!region->isScalar())
          region->setRegion(*this, getRegionScalar());
      }
    }

    if (src0Opnd->isImm() || (src0Opnd->isSrcRegRegion() &&
                              (src0Opnd->asSrcRegRegion()->isScalar()))) {
      dstOpnd->setType(*this, exsize == 32 ? Type_UD : Type_UW);
      if (emask == vISA_EMASK_M5_NM) {
        // write to f0.1/f1.1 instead
        vISA_ASSERT_INPUT(dstOpnd->getTopDcl()->getNumberFlagElements() == 32,
                     "Dst must have 32 flag elements");
        dstOpnd = createDstWithNewSubRegOff(dstOpnd, 1);
      }
      createInst(predOpnd, G4_mov, NULL, saturate, g4::SIMD1, dstOpnd, src0Opnd,
                 NULL, InstOpt_WriteEnable, true);
    } else if (src0Opnd->isSrcRegRegion() &&
               src0Opnd->asSrcRegRegion()->isScalar() == false) {
      G4_DstRegRegion *null_dst_opnd = createNullDst(Type_UD);
      condMod = createCondMod(
          Mod_ne, dstOpnd->asDstRegRegion()->getBase()->asRegVar(), 0);

      createInst(predOpnd, G4_and, condMod, saturate, exsize, null_dst_opnd,
                 src0Opnd, createImm(1, Type_UW), inst_opt, true);
    } else {
      return VISA_FAILURE;
    }
  } else if (opcode == ISA_FCVT) {
    (void)createInst(nullptr, G4_fcvt, nullptr, saturate, exsize, dstOpnd,
                     src0Opnd, nullptr, inst_opt, true);
  } else {
    if (opcode == ISA_FMINMAX) {
      condMod =
          createCondMod(subOpcode == CISA_DM_FMAX ? Mod_ge : Mod_l, nullptr, 0);
    }

    if (opcode == ISA_MOV && src0Opnd->isSrcRegRegion() &&
        src0Opnd->asSrcRegRegion()->isFlag()) {
      // src0 is a flag
      // mov (1) dst src0<0;1:0>:uw (ud if flag has 32 elements)
      G4_Declare *flagDcl = src0Opnd->getTopDcl();
      src0Opnd->asSrcRegRegion()->setType(
          *this, flagDcl->getNumberFlagElements() > 16 ? Type_UD : Type_UW);
    }

    createInst(predOpnd, GetGenOpcodeFromVISAOpcode(opcode), condMod, saturate,
               exsize, dstOpnd, src0Opnd, src1Opnd, inst_opt, true);
  }

  return VISA_SUCCESS;
}
