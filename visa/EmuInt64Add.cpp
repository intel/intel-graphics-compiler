/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Assertions.h"
#include "DebugInfo.h"
#include "G4_Verifier.hpp"
#include "EmuInt64Add.h"
#include "Optimizer.h"
#include "PointsToAnalysis.h"
#include "visa_wa.h"

using namespace vISA;

void EmuInt64Add::transform() {
  for (auto bb : kernel.fg) {
    for (auto it = bb->begin(), itEnd = bb->end(); it != itEnd; ++it) {
      G4_INST *inst = *it;

      if (inst->opcode() != G4_add) {
        continue;
      }

      G4_Operand *dst = inst->getDst();
      if (!(dst->getType() == Type_UQ || dst->getType() == Type_Q)) {
        continue;
      }

      vISA_ASSERT(inst->getExecSize() == g4::SIMD1,
          "Only handle scalar operation for now");

      // Assume addend0->getTypeSize() >= addend1->getTypeSize()
      bool needSwap = inst->getSrc(0)->getTypeSize() < inst->getSrc(1)->getTypeSize();
      auto addend0 = !needSwap ? inst->getSrc(0) : inst->getSrc(1);
      auto addend1 = !needSwap ? inst->getSrc(1) : inst->getSrc(0);
      auto options = inst->getOption();
      auto predicate = inst->getPredicate();
      bool isSigned = IS_SIGNED_INT(addend1->getType());
      bool is32Bit = addend1->getTypeSize() <= 4;
      G4_Type newType = Type_UD;
      G4_Declare *addend0Dcl = addend0->getBase()->asRegVar()->getDeclare();
      G4_Declare *dstDcl = dst->getBase()->asRegVar()->getDeclare();

      if (addend1->getTypeSize() < 4 && isSigned) {
        // create mov for sign extention with smaller types ( < 32 bit )
        G4_Declare *dcl = builder.createTempVar(
            g4::SIMD1, Type_D, Any);
        G4_DstRegRegion *tmpDst = builder.createDstRegRegion(dcl, 1);
        auto movInst = builder.createMov(g4::SIMD1, tmpDst,
            addend1->isImm() ? addend1 : builder.duplicateOperand(addend1)->asSrcRegRegion(),
            options, false);
        addend1 = builder.createSrc(
            tmpDst->getBase(),
            0, 0,
            builder.getRegionScalar(), Type_D);
        bb->insertBefore(it, movInst);
      }

      // Create split region with 32-bit type
      G4_DstRegRegion *dstRegion = createSplitDstRegion(dstDcl, newType);
      G4_SrcRegRegion *srcRegion = createSplitSrcRegion(addend0Dcl, newType);

      // Before transformation:
      //     dst = add addend0, addend1
      // After transformation (is32Bit && !isSigned):
      //   loDst = addc loAddend0, addend1
      //   hiDst = add carry, hiAddend0
      // After transformation (is32Bit && isSigned):
      //   loDst = addc loAddend0, addend1
      //   signExt = asr addend1, 31
      //   hiDst = add3 carry, hiAddend0, signExt
      // After transformation (!is32Bit):
      //   loDst = addc loAddend0, loAddend1
      //   hiDst = add3 carry, hiAddend0, hiAddend1
      G4_Operand *hiAddend1 = nullptr;
      G4_SrcRegRegion *loAddend1 = nullptr;
      if (!is32Bit) {
        G4_Declare *addend1Dcl = addend1->getBase()->asRegVar()->getDeclare();
        loAddend1 = createSplitSrcRegion(addend1Dcl, newType);
        hiAddend1 = builder.createSrc(
            loAddend1->getBase(),
            0, 1,
            loAddend1->getRegion(), newType);
      }
      G4_SrcRegRegion *addcAddend1 =
        (is32Bit) ? builder.duplicateOperand(addend1)->asSrcRegRegion() : loAddend1;
      if (!addend1->isImm() && IS_SIGNED_INT(addcAddend1->getType()))
        addcAddend1->setType(builder, getUnsignedType(TypeSize(addcAddend1->getType())));
      auto loDst = dstRegion;
      auto loAddend0 = srcRegion;
      auto addcInst = builder.createBinOp(
          G4_addc, g4::SIMD1, loDst, loAddend0,
          addend1->isImm() ? addend1 : addcAddend1, options, false);
      addcInst->setPredicate(predicate);
      G4_DstRegRegion *dstAcc0 =
        builder.createDst(builder.phyregpool.getAcc0Reg(), 0, 0, 1, Type_UD);
      addcInst->setImplAccDst(dstAcc0);
      addcInst->setOptionOn(InstOpt_AccWrCtrl);
      bb->insertBefore(it, addcInst);

      // Emit asr instruction with signed addition
      G4_Operand *signExtAsSrc = nullptr;
      if (is32Bit && isSigned) {
        G4_DstRegRegion *signExt = nullptr;
        auto imm31 = builder.createImm(31, Type_W);
        G4_Declare *dcl = builder.createTempVar(
            g4::SIMD1, newType, Any);
        signExt = builder.createDstRegRegion(dcl, 1);
        auto asrInst = builder.createBinOp(
            G4_asr, g4::SIMD1, signExt, builder.duplicateOperand(addend1),
            imm31, options, false);
        signExtAsSrc = builder.createSrc(
            signExt->getBase(), 0, 0,
            builder.getRegionScalar(), Type_UD);
        bb->insertBefore(it, asrInst);
      }

      G4_DstRegRegion *hiDst = builder.createDst(
          dstRegion->getBase(),
          0, 1,
          1, newType);
      G4_Operand *hiAddend0 = builder.createSrc(
          srcRegion->getBase(),
          0, 1,
          srcRegion->getRegion(), newType);

      G4_Operand *carry = builder.createSrc(
          dstAcc0->getBase(), 0, 0,
          builder.getRegionScalar(), Type_UD);

      inst->setDest(hiDst);
      inst->setSrc(carry, 0);
      inst->setSrc(hiAddend0, 1);

      // Alter to be add3
      if (!(is32Bit && !isSigned)) {
        bool supportAdd3 = builder.getPlatform() >= Xe_XeHPSDV;
        auto src2 = (is32Bit) ? signExtAsSrc : hiAddend1;
        if (supportAdd3) {
          inst->setOpcode(G4_add3);
          inst->setSrc(src2, 2);
        } else {
          auto newDst = builder.createDstRegRegion(
              builder.createTempVar(g4::SIMD1, newType, Any), 1);
          auto addInst = builder.createBinOp(
              G4_add, g4::SIMD1, newDst, builder.duplicateOperand(inst->getSrc(1)),
              src2, options, false);
          auto newSrc = builder.createSrc(
              newDst->getBase(), 0, 0, builder.getRegionScalar(), Type_UD);
          bb->insertBefore(it, addInst);
          inst->setSrc(newSrc, 1);
        }
      }
    }
  }
}

//
// single entry point for EmuInt64Add
//
void EmulateInt64Add(IR_Builder &builder, G4_Kernel &kernel) {
  EmuInt64Add emulate(builder, kernel);
  // Perform transformation when no Int64 Add support
  if (!builder.hasInt64Add()) {
    emulate.transform();
  }
}
