/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../Timer.h"
#include "BuildIR.h"

using namespace vISA;

int IR_Builder::translateVISARawSendInst(
    G4_Predicate *predOpnd, VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
    uint8_t modifiers, unsigned int exDesc, uint8_t numSrc, uint8_t numDst,
    G4_Operand *msgDescOpnd, G4_SrcRegRegion *msgOpnd,
    G4_DstRegRegion *dstOpnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize exsize = G4_ExecSize(Get_VISA_Exec_Size(executionSize));
  G4_InstOpts inst_opt = Get_Gen4_Emask(emask, exsize);

  if (msgDescOpnd->isSrcRegRegion()) {
    // mov (1) a0.0<1>:ud src<0;1,0>:ud {NoMask}
    G4_DstRegRegion *dstOpnd = createDstRegRegion(builtinA0, 1);
    createMov(g4::SIMD1, dstOpnd, msgDescOpnd, InstOpt_WriteEnable, true);
    msgDescOpnd = createSrcRegRegion(builtinA0, getRegionScalar());
  }

  uint32_t desc = 0;
  bool isRead = true, isWrite = true, isValidFuncCtrl = true;
  if (msgDescOpnd->isImm()) {
    desc = (uint32_t)msgDescOpnd->asImm()->getImm();
  } else {
    desc = G4_SendDescRaw::createDesc(0, false, numSrc, numDst);
    isValidFuncCtrl = false;
  }

  // bit[0-3] of the exDesc (always imm) holds the SFID
  G4_SendDescRaw *sendMsgDesc = createSendMsgDesc(
      intToSFID(exDesc & 0xF, getPlatform()), desc, exDesc, 0,
      getSendAccessType(isRead, isWrite), nullptr, isValidFuncCtrl);

  // sanity check on srcLen/dstLen moved to ISA verifier

  G4_InstSend *inst =
    createSendInst(predOpnd, (modifiers & 1) ? G4_sendc : G4_send, exsize,
                   dstOpnd, msgOpnd, msgDescOpnd, inst_opt, sendMsgDesc, true);
  // for legacy platforms (pre-TGL) where ExDesc has EOT bit
  // (also possible newer platforms interpret ExDesc[5] as EOT in raw send)
  bool isEotInExDesc = exDesc & 0x20;
  if (isEotInExDesc) {
    inst->setEOT();
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISARawSendsInst(
    G4_Predicate *predOpnd, VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
    uint8_t modifiers, G4_Operand *ex, uint8_t numSrc0, uint8_t numSrc1,
    uint8_t numDst, G4_Operand *msgDescOpnd, G4_Operand *src0, G4_Operand *src1,
    G4_DstRegRegion *dstOpnd, unsigned ffid, bool hasEOT) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize exsize = G4_ExecSize(Get_VISA_Exec_Size(executionSize));
  const G4_InstOpts inst_opt = Get_Gen4_Emask(emask, exsize);

  if (msgDescOpnd->isSrcRegRegion()) {
    // mov (1) a0.0<1>:ud src<0;1,0>:ud {NoMask}
    G4_DstRegRegion *dstOpnd = createDstRegRegion(builtinA0, 1);
    createMov(g4::SIMD1, dstOpnd, msgDescOpnd, InstOpt_WriteEnable, true);
    msgDescOpnd = createSrcRegRegion(builtinA0, getRegionScalar());
  }

  uint32_t exDescVal = 0;
  G4_SrcRegRegion *temp_exdesc_src = nullptr;
  if (ex->isImm()) {
    exDescVal = (unsigned)ex->asImm()->getInt();
    if (VISA_WA_CHECK(getPWaTable(), Wa_14020375314)) {
      vISA_ASSERT_INPUT(
          ((exDescVal >> 12) & 0xF) != 0xB,
          "raw_sends: ExDesc[15:12] must not be 0xB on this platform");
      if (((exDescVal >> 12) & 0xF) == 0xB) {
        return VISA_FAILURE;
      }
    }
  }

  // bit [6:10] store the extended message length, and when it's >= 16 we have
  // to use indirect
  uint32_t extLength = (exDescVal >> 6) & 0x1F;
  if (ex->isSrcRegRegion() || extLength >= 16) {
    // mov (1) a0.2<1>:ud src<0;1,0>:ud {NoMask}
    // to hold the dynamic ext msg descriptor
    G4_DstRegRegion *exDescDst = createDstRegRegion(getBuiltinA0Dot2(), 1);
    createMov(g4::SIMD1, exDescDst, ex, InstOpt_WriteEnable, true);
    temp_exdesc_src = createSrcRegRegion(getBuiltinA0Dot2(), getRegionScalar());

    if (exDescVal == 0) {
      exDescVal = G4_SendDescRaw::createExtDesc(intToSFID(ffid, getPlatform()),
                                                false, numSrc1);
    }
  }

  uint32_t descVal = 0;
  bool isValidFuncCtrl = true;
  if (msgDescOpnd->isImm()) {
    descVal = (uint32_t)msgDescOpnd->asImm()->getImm();
  } else {
    descVal = G4_SendDescRaw::createDesc(0, false, numSrc0, numDst);
    isValidFuncCtrl = false;
  }

  G4_SendDescRaw *sendMsgDesc = createSendMsgDesc(
      intToSFID(ffid, getPlatform()), descVal, exDescVal, numSrc1,
      SendAccess::READ_WRITE, nullptr, isValidFuncCtrl);

  vISA_ASSERT(sendMsgDesc->MessageLength() == numSrc0,
               "message length mismatch for raw sends");
  if (!dstOpnd->isNullReg()) {
    vISA_ASSERT(sendMsgDesc->ResponseLength() <= numDst,
                 "response length mismatch for raw sends");
  }
  vISA_ASSERT(sendMsgDesc->extMessageLength() <= numSrc1,
               "extended message length mismatch for raw sends");

  G4_InstSend* sendInst = createSplitSendInst(predOpnd,
                      (modifiers & 1) ? G4_sendsc : G4_sends, exsize,
                      dstOpnd, src0->asSrcRegRegion(), src1->asSrcRegRegion(),
                      msgDescOpnd, inst_opt, sendMsgDesc, temp_exdesc_src,
                      true);
  if (hasEOT) {
    sendInst->setEOT();
  }
  if (getOption(vISA_renderTargetWriteSendReloc) &&
    dstOpnd->isNullReg() &&
    SFID::DP_RC == intToSFID(ffid, getPlatform()))
  {
    std::string symbolName{ "RTW_SEND" };
    RelocationEntry::createRelocation(kernel, *sendInst, 0, symbolName,
        GenRelocType::R_SEND);
  }

  return VISA_SUCCESS;
}

