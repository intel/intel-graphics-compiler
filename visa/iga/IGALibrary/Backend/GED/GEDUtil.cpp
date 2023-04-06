/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GEDUtil.hpp"
#include "GEDToIGATranslation.hpp"
#include "IGAToGEDTranslation.hpp"

#include "../../bits.hpp"

using namespace iga;

static const GED_RETURN_VALUE
constructPartialGEDSendInstruction(ged_ins_t *ins, GED_MODEL gedP,
                                   GED_OPCODE op, bool supportsExMsgDesc,
                                   uint32_t exMsgDesc, uint32_t msgDesc) {
  GED_RETURN_VALUE status = GED_InitEmptyIns(gedP, ins, op);

  status = GED_SetExecSize(ins, 16);

  if (supportsExMsgDesc && status == GED_RETURN_VALUE_SUCCESS) {
    status = GED_SetExDescRegFile(ins, GED_REG_FILE_IMM);
  }

  if (status == GED_RETURN_VALUE_SUCCESS) {
    status = GED_SetExMsgDescImm(ins, exMsgDesc);
  }

  if (status == GED_RETURN_VALUE_SUCCESS) {
    status = GED_SetDescRegFile(ins, GED_REG_FILE_IMM);
  }

  if (status == GED_RETURN_VALUE_SUCCESS) {
    status = GED_SetMsgDesc(ins, msgDesc);
  }
  return status;
}

iga::SFMessageType iga::getMessageType(Platform p, SFID sfid, uint32_t desc) {
  GED_SFID gedSFID = lowerSFID(sfid);

  GED_RETURN_VALUE getRetVal = GED_RETURN_VALUE_INVALID_FIELD;
  GED_MESSAGE_TYPE msgType = GED_MESSAGE_TYPE_INVALID;
  GED_MODEL gedP = lowerPlatform(p);


  if (gedSFID != GED_SFID_INVALID && gedSFID != GED_SFID_NULL) {
    switch (gedSFID) {
    case GED_SFID_SAMPLER: ///< all
      msgType = GED_GetMessageTypeDP_SAMPLER(desc, gedP, &getRetVal);
      break;
    case GED_SFID_GATEWAY: ///< all
      break;
      /// includes: GEN11
    case GED_SFID_DP_DC2: ///< GEN10, GEN9
      msgType = GED_GetMessageTypeDP_DC2(desc, gedP, &getRetVal);
      break;
    case GED_SFID_DP_RC: ///< all
      msgType = GED_GetMessageTypeDP_RC(desc, gedP, &getRetVal);
      break;
    case GED_SFID_URB: ///< all
      break;
    case GED_SFID_SPAWNER: ///< all
      break;
    case GED_SFID_VME: ///< all
      break;
      /// includes: GEN11
    case GED_SFID_DP_DCRO: ///< GEN10, GEN9
      msgType = GED_GetMessageTypeDP_DCRO(desc, gedP, &getRetVal);
      break;
    case GED_SFID_DP_DC0: ///< all
      if (p <= iga::Platform::GEN7P5) {
        // IVB,HSW
        msgType = GED_GetMessageTypeDP_DC0(desc, gedP, &getRetVal);
      } else {
        // Starting with BDW
        if (GED_GetMessageTypeDP0Category(desc, gedP, &getRetVal) == 0) {
          msgType = GED_GetMessageTypeDP_DC0Legacy(desc, gedP, &getRetVal);
        } else {
          msgType =
              GED_GetMessageTypeDP_DC0ScratchBlock(desc, gedP, &getRetVal);
        }
      }
      break;
    case GED_SFID_PI: ///< all
      break;
      /// includes: GEN11
    case GED_SFID_DP_DC1: ///< GEN10, GEN7.5, GEN8, GEN8.1, GEN9
      msgType = GED_GetMessageTypeDP_DC1(desc, gedP, &getRetVal);
      break;
      /// includes: GEN11
    case GED_SFID_CRE: ///< GEN10, GEN7.5, GEN8, GEN8.1, GEN9
      break;
    case GED_SFID_DP_SAMPLER: ///< GEN7, GEN7.5, GEN8, GEN8.1
      msgType = GED_GetMessageTypeDP_SAMPLER(desc, gedP, &getRetVal);
      break;
    case GED_SFID_DP_CC: ///< GEN7, GEN7.5, GEN8, GEN8.1
      msgType = GED_GetMessageTypeDP_CC(desc, gedP, &getRetVal);
      break;
    default:
      break;
    }
  }

  if (msgType == GED_MESSAGE_TYPE_INVALID ||
      getRetVal != GED_RETURN_VALUE_SUCCESS) {
    return SFMessageType::INVALID;
  } else {
    return translate(msgType);
  }
}

static uint32_t getSplitSendMsgLength(uint32_t exDesc) {
  return getBits<uint32_t>(exDesc, 6, 4);
}

uint32_t iga::getMessageLengths(Platform p, const OpSpec &os, uint32_t exDesc,
                                uint32_t desc, uint32_t *mLen, uint32_t *emLen,
                                uint32_t *rLen) {
  GED_MODEL gedP = lowerPlatform(p);
  GED_OPCODE gedOp = GED_OPCODE_INVALID;
  if (os.isSendFormat()) {
    gedOp = GED_OPCODE_send;
  } else if (os.isSendsFormat()) {
    gedOp = GED_OPCODE_sends;
  }

  if (gedOp == GED_OPCODE_INVALID)
    return 0;

  ged_ins_t gedInst;
  auto getRetVal = constructPartialGEDSendInstruction(
      &gedInst, gedP, gedOp, os.isSendsFormat(), exDesc, desc);

  const uint32_t INVALID =
      ((uint32_t)0xFFFFFFFFF); // TODO: include macro in iga.h?

  uint32_t n = 0; // count successes

  *mLen = GED_GetMessageLength(desc, gedP, &getRetVal);
  if (getRetVal != GED_RETURN_VALUE_SUCCESS) { // TODO: what do I do if return
                                               // is not success?
    *mLen = INVALID;
  } else
    n++;

  if (os.isSendsFormat()) {
    *emLen = GED_GetExMsgLength(&gedInst, &getRetVal);
    if (getRetVal != GED_RETURN_VALUE_SUCCESS) {
      *emLen = getSplitSendMsgLength(exDesc);
    }
    n++;
  }

  *rLen = GED_GetResponseLength(desc, gedP, &getRetVal);
  if (getRetVal != GED_RETURN_VALUE_SUCCESS) { // TODO: what do I do if return
                                               // is not success?
    *rLen = INVALID;
  } else
    n++;

  return n;
}
