/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../Timer.h"
#include "BuildIR.h"

using namespace vISA;

static const unsigned MESSAGE_PRECISION_SUBTYPE_OFFSET = 30;
static const unsigned SIMD_MODE_2_OFFSET = 29;

static bool isSamplerMsgWithPO(VISASampler3DSubOpCode samplerOp) {
  switch (samplerOp) {
  case VISA_3D_SAMPLE_PO:
  case VISA_3D_SAMPLE_PO_B:
  case VISA_3D_SAMPLE_PO_L:
  case VISA_3D_SAMPLE_PO_C:
  case VISA_3D_SAMPLE_PO_D:
  case VISA_3D_SAMPLE_PO_L_C:
  case VISA_3D_SAMPLE_PO_LZ:
  case VISA_3D_SAMPLE_PO_C_LZ:
  case VISA_3D_GATHER4_PO_PACKED:
  case VISA_3D_GATHER4_PO_PACKED_L:
  case VISA_3D_GATHER4_PO_PACKED_B:
  case VISA_3D_GATHER4_PO_PACKED_I:
  case VISA_3D_GATHER4_PO_PACKED_C:
  case VISA_3D_GATHER4_PO_PACKED_I_C:
  case VISA_3D_GATHER4_PO_PACKED_L_C:
    return true;
  default:
    return false;
  }
  return false;
}

[[maybe_unused]]
static MsgOp ConvertSamplerOpToMsgOp(VISASampler3DSubOpCode op) {
  switch (op) {
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE:
    return MsgOp::SAMPLE;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_B:
    return MsgOp::SAMPLE_B;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_L:
    return MsgOp::SAMPLE_L;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_C:
    return MsgOp::SAMPLE_C;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_D:
    return MsgOp::SAMPLE_D;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_B_C:
    return MsgOp::SAMPLE_B_C;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_L_C:
    return MsgOp::SAMPLE_L_C;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4:
    return MsgOp::GATHER4;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4_L:
    return MsgOp::GATHER4_L;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4_B:
    return MsgOp::GATHER4_B;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4_I:
    return MsgOp::GATHER4_I;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4_C:
    return MsgOp::GATHER4_C;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4_I_C:
    return MsgOp::GATHER4_I_C;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4_L_C:
    return MsgOp::GATHER4_L_C;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4_PO_PACKED:
    // case VISASampler3DSubOpCode::VISA_3D_GATHER4_PO:
    return MsgOp::GATHER4_PO;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4_PO_PACKED_L:
    return MsgOp::GATHER4_PO_L;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4_PO_PACKED_B:
    return MsgOp::GATHER4_PO_B;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4_PO_PACKED_I:
    return MsgOp::GATHER4_PO_I;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4_PO_PACKED_C:
    // case VISASampler3DSubOpCode::VISA_3D_GATHER4_PO_C:
    return MsgOp::GATHER4_PO_C;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4_PO_PACKED_I_C:
    return MsgOp::GATHER4_PO_I_C;
  case VISASampler3DSubOpCode::VISA_3D_GATHER4_PO_PACKED_L_C:
    return MsgOp::GATHER4_PO_L_C;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_D_C_MLOD:
    return MsgOp::SAMPLE_D_C_MLOD;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_MLOD:
    return MsgOp::SAMPLE_MLOD;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_C_MLOD:
    return MsgOp::SAMPLE_C_MLOD;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_D_C:
    return MsgOp::SAMPLE_D_C;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_LZ:
    return MsgOp::SAMPLE_LZ;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_C_LZ:
    return MsgOp::SAMPLE_C_LZ;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_PO:
    return MsgOp::SAMPLE_PO;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_PO_B:
    return MsgOp::SAMPLE_PO_B;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_PO_L:
    return MsgOp::SAMPLE_PO_L;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_PO_C:
    return MsgOp::SAMPLE_PO_C;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_PO_L_C:
    return MsgOp::SAMPLE_PO_L_C;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_PO_D:
    return MsgOp::SAMPLE_PO_D;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_PO_LZ:
    return MsgOp::SAMPLE_PO_LZ;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLE_PO_C_LZ:
    return MsgOp::SAMPLE_PO_C_LZ;
  case VISASampler3DSubOpCode::VISA_3D_LD_LZ:
    return MsgOp::LD_LZ;
  case VISASampler3DSubOpCode::VISA_3D_LD_L:
    return MsgOp::LD_L;
  case VISASampler3DSubOpCode::VISA_3D_LD:
    return MsgOp::LD;
  case VISASampler3DSubOpCode::VISA_3D_LD2DMS_W:
    return MsgOp::LD_2DMS_W;
  case VISASampler3DSubOpCode::VISA_3D_LD_MCS:
    return MsgOp::LD_MCS;
  case VISASampler3DSubOpCode::VISA_3D_SAMPLEINFO:
    return MsgOp::SAMPLE_INFO;
  case VISASampler3DSubOpCode::VISA_3D_RESINFO:
    return MsgOp::RESINFO;
  case VISASampler3DSubOpCode::VISA_3D_LOD:
    return MsgOp::LOD;
  default:
    return MsgOp::INVALID;
  }
}

uint32_t IR_Builder::createSamplerMsgDesc(VISASampler3DSubOpCode samplerOp,
                                          bool isNativeSIMDSize,
                                          bool isFP16Return,
                                          bool isFP16Input) const {
  // Now create message descriptor
  // 7:0 - BTI
  // 11:8 - Sampler Index
  // 16:12 - Message Type
  // 18:17 - SIMD Mode[0:1]
  // 19 - Header Present
  // 24:20 - Response Length
  // 28:25 - Message Length
  // 29 - SIMD Mode[2]
  // 30 - Return Format
  // 31 - CPS Message LOD Compensation Enable
  // We only set message type, SIMD mode, and return format here.  The other
  // fields are set in createSendInst as they are common with other send
  // messages
  uint32_t fc = 0;

  fc |= ((uint32_t)samplerOp & 0x1f) << 12;

  if (!getOption(vISA_EnableProgrammableOffsetsMessageBitInHeader)) {
    // set bit 31 for sampler messages with positional offsets
    if (isSamplerMsgWithPO(samplerOp)) {
      fc |= 1UL << 31;
    }
  }

  if (isNativeSIMDSize) {
    fc |= (1 << 17);
  } else {
    fc |= (2 << 17);
  }

  if (isFP16Return) {
    // 16-bit return type.  Note that this doesn't change the return length
    fc |= (1 << MESSAGE_PRECISION_SUBTYPE_OFFSET);
  }

  if (isFP16Input) {
    fc |= (1 << SIMD_MODE_2_OFFSET);
  }

  return fc;
}

int IR_Builder::translateVISASampleInfoInst(VISA_Exec_Size executionSize,
                                            VISA_EMask_Ctrl emask,
                                            ChannelMask chMask,
                                            G4_Operand *surface,
                                            G4_DstRegRegion *dst) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize execSize{Get_VISA_Exec_Size(executionSize)};
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, execSize);
  VISAChannelMask channels = chMask.getAPI();
  bool useFakeHeader =
      (getPlatform() < GENX_SKL) ? false : (channels == CHANNEL_MASK_R);
  bool preEmption = forceSamplerHeader();
  bool forceSplitSend = shouldForceSplitSend(surface);
  bool useHeader = true;
  bool forceHeader = false;
  // SAMPLEINFO has 0 parameters so its only header

  unsigned int numRows = 1;

  G4_Declare *msg = NULL;
  G4_SrcRegRegion *m0 = NULL;

  if (!useFakeHeader || forceSplitSend || preEmption || forceHeader) {
    msg = getSamplerHeader(false /*isBindlessSampler*/,
                           false /*samperIndexGE16*/);

    unsigned int secondDword = chMask.getHWEncoding() << 12;


    G4_Imm *immOpndSecondDword = createImm(secondDword, Type_UD);

    // mov (1) msg(0,2) immOpndSecondDword
    auto payloadDstRgn = createDst(msg->getRegVar(), 0, 2, 1, Type_UD);

    G4_INST *movInst = createMov(g4::SIMD1, payloadDstRgn, immOpndSecondDword,
                                 InstOpt_NoOpt, true);
    movInst->setOptionOn(InstOpt_WriteEnable);

    m0 = createSrcRegRegion(msg, getRegionStride1());
  } else {
    useHeader = false;
    msg = createTempVar(getNativeExecSize(), Type_UD, getGRFAlign());
    G4_DstRegRegion *dst = createDst(msg->getRegVar(), 0, 0, 1, Type_UD);
    G4_Imm *src0Imm = createImm(0, Type_UD);
    (void)createMov(getNativeExecSize(), dst, src0Imm, InstOpt_WriteEnable,
                    true);
    m0 = createSrc(msg->getRegVar(), 0, 0, getRegionStride1(), Type_UD);
  }
  // Now create message descriptor
  // 7:0 - BTI
  // 11:8 - Sampler Index
  // 16:12 - Message Type
  // 18:17 - SIMD Mode
  // 19 - Header Present
  // 24:20 - Response Length
  // 28:25 - Message Length
  // 29 - SIMD Mode
  // 30 - Return Format
  // 31 - CPS Message LOD Compensation Enable
  unsigned int fc = 0;

  fc |= ((unsigned int)VISA_3D_SAMPLEINFO & 0x1f) << 12;

  if (execSize == getNativeExecSize()) {
    fc |= (1 << 17);
  } else {
    fc |= (2 << 17);
  }

  uint32_t retSize =
      (execSize == getNativeExecSize() ? chMask.getNumEnabledChannels()
                                       : chMask.getNumEnabledChannels() * 2);

  if (forceSplitSend) {
    createSplitSendInst(NULL, dst, m0, numRows, createNullSrc(Type_UD), 0,
                        retSize, execSize, fc, SFID::SAMPLER, useHeader,
                        SendAccess::READ_ONLY, surface, NULL, instOpt, false);
  } else {
    createSendInst(NULL, dst, m0, numRows, retSize, execSize, fc, SFID::SAMPLER,
                   useHeader, SendAccess::READ_ONLY, surface, NULL, instOpt,
                   false);
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISAResInfoInst(
    VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask, ChannelMask chMask,
    G4_Operand *surface, G4_SrcRegRegion *lod, G4_DstRegRegion *dst) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize execSize{Get_VISA_Exec_Size(executionSize)};
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, execSize);
  // For SKL if channels are continuous don't need header

  VISAChannelMask channels = chMask.getAPI();
  bool preEmption = forceSamplerHeader();
  bool useHeader =
      preEmption || (getPlatform() < GENX_SKL)
          ? channels != CHANNEL_MASK_RGBA
          : (channels != CHANNEL_MASK_R && channels != CHANNEL_MASK_RG &&
             channels != CHANNEL_MASK_RGB && channels != CHANNEL_MASK_RGBA);

  // Setup number of rows = (header + lod) by default
  unsigned int numRows = (execSize == getNativeExecSize() ? 1 : 2);
  if (useHeader) {
    numRows++;
  }
  unsigned int regOff = 0;
  uint32_t returnLength =
      (execSize == getNativeExecSize() ? chMask.getNumEnabledChannels()
                                       : chMask.getNumEnabledChannels() * 2);

  bool useSplitSend = useSends();

  G4_Declare *msg = NULL;
  G4_Declare *payloadUD = NULL;
  if (useSplitSend) {
    if (useHeader) {
      --numRows;
    }
    unsigned int numElts = numRows * numEltPerGRF<Type_UB>() / TypeSize(Type_F);
    msg = getSamplerHeader(false /*isBindlessSampler*/,
                           false /*samperIndexGE16*/);
    payloadUD = createSendPayloadDcl(numElts, Type_UD);
  } else {
    unsigned int numElts = numRows * numEltPerGRF<Type_UB>() / TypeSize(Type_F);
    msg = createSendPayloadDcl(numElts, Type_UD);
    payloadUD = createSendPayloadDcl(
        numElts - (useHeader ? getGenxSamplerIOSize() : 0), Type_UD);
    payloadUD->setAliasDeclare(msg, useHeader ? numEltPerGRF<Type_UB>() : 0);

    if (useHeader) {
      // Both SAMPLEINFO and RESINFO use header
      createMovR0Inst(msg, 0, 0, true);
    }
  }

  if (useHeader) {
    unsigned int secondDword = 0;
    secondDword |= (chMask.getHWEncoding() << 12);


    G4_Imm *immOpndSecondDword = createImm(secondDword, Type_UD);

    // mov (1) msg(0,2) immOpndSecondDword
    auto payloadDstRgn = createDst(msg->getRegVar(), 0, 2, 1, Type_UD);

    G4_INST *movInst = createMov(g4::SIMD1, payloadDstRgn, immOpndSecondDword,
                                 InstOpt_NoOpt, true);
    movInst->setOptionOn(InstOpt_WriteEnable);
  }

  // Copy over lod vector operand to payload's 1st row
  Copy_SrcRegRegion_To_Payload(payloadUD, regOff, lod, execSize,
                               instOpt | InstOpt_BreakPoint);

  // Now create message descriptor
  // 7:0 - BTI
  // 11:8 - Sampler Index
  // 16:12 - Message Type
  // 18:17 - SIMD Mode
  // 19 - Header Present
  // 24:20 - Response Length
  // 28:25 - Message Length
  // 29 - SIMD Mode
  // 30 - Return Format
  // 31 - CPS Message LOD Compensation Enable
  unsigned int fc = 0;

  fc |= ((unsigned int)VISA_3D_RESINFO & 0x1f) << 12;

  if (execSize == getNativeExecSize()) {
    fc |= (1 << 17);
  } else {
    fc |= (2 << 17);
  }

  if (useSplitSend) {
    G4_SrcRegRegion *m0 = nullptr;
    G4_SrcRegRegion *m1 = nullptr;
    unsigned int src0Size = 0;
    unsigned int src1Size = 0;

    if (useHeader) {
      m0 = createSrcRegRegion(msg, getRegionStride1());
      m1 = createSrcRegRegion(payloadUD, getRegionStride1());
      src0Size = 1;
      src1Size = numRows;
    } else {
      m0 = createSrcRegRegion(payloadUD, getRegionStride1());
      m1 = createNullSrc(Type_UD);
      src0Size = numRows;
      src1Size = 0;
    }
    createSplitSendInst(NULL, dst, m0, src0Size, m1, src1Size, returnLength,
                        execSize, fc, SFID::SAMPLER, useHeader,
                        SendAccess::READ_ONLY, surface, NULL, instOpt, false);
  } else {
    G4_SrcRegRegion *m = createSrcRegRegion(msg, getRegionStride1());
    createSendInst(NULL, dst, m, numRows, returnLength, execSize, fc,
                   SFID::SAMPLER, useHeader, SendAccess::READ_ONLY, surface,
                   NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

// generate a URB_SIMD8* message
// urbHandle -- 1 GRF holding 8 URB handles.  This is the header of the message
// perSlotOffset -- 1 GRF holding 8 DWord offsets.  If present, it must be
// immediately after the header channelMask -- 1 GRF holding 8 8-bit masks.  In
// vISA spec they have constant values and must be
//                identical.  If present,  occurs after the per slot message
//                phase if the per slot message phase exists else it occurs
//                after the header.

int IR_Builder::translateVISAURBWrite3DInst(
    G4_Predicate *pred, VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
    uint8_t numOut, uint16_t globalOffset, G4_SrcRegRegion *channelMask,
    G4_SrcRegRegion *urbHandle, G4_SrcRegRegion *perSlotOffset,
    G4_SrcRegRegion *vertexData) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize execSize{Get_VISA_Exec_Size(executionSize)};
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, execSize);

  if (numOut == 0) {
    vISA_ASSERT_INPUT(vertexData->isNullReg(),
                      "vertex payload must be null ARF when numOut is 0");
  }

  // header + channelMask + numOut
  unsigned int numRows = 2 + numOut;
  const bool useHeader = true;
  bool usePerSlotIndex = false;
  bool useChannelMask = true;

  if (!perSlotOffset->isNullReg()) {
    usePerSlotIndex = true;
    numRows++;
  }

  if (channelMask->isNullReg()) {
    useChannelMask = false;
    numRows--;
  }

  bool useSplitSend = useSends();
  // So far, we don't have a obvious cut except for header. As the result,
  // split-send is disabled once there's no header in the message.
  if (!useHeader)
    useSplitSend = false;

  if (numOut == 0) {
    // no split send if payload is null
    useSplitSend = false;
  }

  // msg is the header for split send, or the entire payload for regular send
  G4_Declare *msg = NULL;
  G4_Declare *payloadF = NULL;
  G4_Declare *payloadD = NULL;
  G4_Declare *payloadUD = NULL;
  if (useSplitSend) {
    vISA_ASSERT_INPUT( useHeader,
            "So far, split-send is only used when header is present!");
    --numRows;
    if (numRows > 0) {
      unsigned int numElts =
          numRows * numEltPerGRF<Type_UB>() / TypeSize(Type_F);
      // we can use the urb handle directly since URB write will not modify its
      // header
      // msg = createSendPayloadDcl(getGenxSamplerIOSize(), Type_UD);
      payloadUD = createSendPayloadDcl(numElts, Type_UD);
      payloadF = createSendPayloadDcl(numElts, Type_F);
      payloadD = createSendPayloadDcl(numElts, Type_D);
      payloadF->setAliasDeclare(payloadUD, 0);
      payloadD->setAliasDeclare(payloadUD, 0);
    }
  } else {
    unsigned int numElts = numRows * numEltPerGRF<Type_UB>() / TypeSize(Type_F);
    msg = createSendPayloadDcl(numElts, Type_UD);
    if (numRows > 1) {
      payloadUD = createSendPayloadDcl(
          numElts - (useHeader ? getGenxSamplerIOSize() : 0), Type_UD);
      payloadF = createSendPayloadDcl(
          numElts - (useHeader ? getGenxSamplerIOSize() : 0), Type_F);
      payloadD = createSendPayloadDcl(
          numElts - (useHeader ? getGenxSamplerIOSize() : 0), Type_D);
      payloadUD->setAliasDeclare(msg, useHeader ? numEltPerGRF<Type_UB>() : 0);
      payloadF->setAliasDeclare(msg, useHeader ? numEltPerGRF<Type_UB>() : 0);
      payloadD->setAliasDeclare(msg, useHeader ? numEltPerGRF<Type_UB>() : 0);
    }
  }

  unsigned int regOff = 0;
  // Setup header
  if (useHeader && msg != NULL) {
    unsigned ignoredOff = 0;
    Copy_SrcRegRegion_To_Payload(msg, ignoredOff, urbHandle, g4::SIMD8,
                                 instOpt);
  }

  if (usePerSlotIndex) {
    Copy_SrcRegRegion_To_Payload(payloadUD, regOff, perSlotOffset, g4::SIMD8,
                                 instOpt);
  }

  if (useChannelMask) {

    // shl (8) M2.0<1>:ud cmask<8;8,1>:ud 0x10:uw
    auto payloadUDRegRgnRow2 =
        createDst(payloadUD->getRegVar(), regOff++, 0, 1, Type_UD);

    createBinOp(G4_shl, g4::SIMD8, payloadUDRegRgnRow2, channelMask,
                createImm(16, Type_UW), instOpt, true);
  }

  G4_Declare *vertexDataDcl =
      numOut == 0 ? NULL : vertexData->getBase()->asRegVar()->getDeclare();

  bool needsDataMove = (!useSplitSend || usePerSlotIndex || useChannelMask);
  if (needsDataMove) {
    // we have to insert moves to make payload contiguous
    unsigned int startSrcRow = vertexData->getRegOff();

    for (int i = 0; i < numOut; i++) {
      G4_DstRegRegion payloadTypedRegRowi(*this, Direct, payloadF->getRegVar(),
                                          regOff++, 0, 1, Type_F);
      G4_DstRegRegion *payloadTypedRegRowRgni =
          createDstRegRegion(payloadTypedRegRowi);

      G4_SrcRegRegion *vertexSrcRegRgnRowi =
          createSrc(vertexDataDcl->getRegVar(), startSrcRow++, 0,
                    getRegionStride1(), Type_F);

      createMov(g4::SIMD8, payloadTypedRegRowRgni, vertexSrcRegRgnRowi, instOpt,
                true);
    }
  } else {
    payloadUD = vertexDataDcl;
  }

  // Msg descriptor
  unsigned int fc = 0;

  fc |= 0x7;

  fc |= (globalOffset << 4);

  if (useChannelMask) {
    fc |= (0x1 << 15);
  }

  if (usePerSlotIndex) {
    fc |= (0x1 << 17);
  }

  if (useSplitSend) {
    G4_SrcRegRegion *m0 = urbHandle;
    G4_SrcRegRegion *m1 = nullptr;

    if (needsDataMove) {
      m1 = createSrcRegRegion(payloadUD, getRegionStride1());
    } else {
      vISA_ASSERT(payloadUD == vertexDataDcl,
                  "If there is no need for data move then payloadUD == "
                  "vertexDataDcl must hold!");

      m1 = createSrc(payloadUD->getRegVar(), vertexData->getRegOff(),
                     vertexData->getSubRegOff(), getRegionStride1(),
                     payloadUD->getElemType());
    }

    createSplitSendInst(pred, createNullDst(Type_UD), m0, 1, m1, numRows, 0,
                        execSize, fc, SFID::URB, useHeader,
                        SendAccess::WRITE_ONLY, NULL, NULL, instOpt, false);
  } else {
    G4_SrcRegRegion *m = createSrcRegRegion(msg, getRegionStride1());
    createSendInst(pred, createNullDst(Type_UD), m, numRows, 0, execSize, fc,
                   SFID::URB, useHeader, SendAccess::WRITE_ONLY, nullptr,
                   nullptr, instOpt, false);
  }
  return VISA_SUCCESS;
}

/*****************************************************************************\
ENUM: EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL
\*****************************************************************************/
enum EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL {
  EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD16_SINGLE_SOURCE = 0,
  EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD16_SINGLE_SOURCE_REPLICATED =
      1,
  EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD8_DUAL_SOURCE_LOW = 2,
  EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD8_DUAL_SOURCE_HIGH = 3,
  EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD8_SINGLE_SOURCE_LOW = 4,
  EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD8_IMAGE_WRITE = 5
};

int IR_Builder::translateVISARTWrite3DInst(
    G4_Predicate *pred, VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
    G4_Operand *surface, G4_SrcRegRegion *r1HeaderOpnd, G4_Operand *rtIndex,
    vISA_RT_CONTROLS cntrls, G4_SrcRegRegion *sampleIndexOpnd,
    G4_Operand *cpsCounter, unsigned int numParms, G4_SrcRegRegion **msgOpnds) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize execSize = toExecSize(executionSize);
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, execSize);
  bool useHeader = false;

  uint8_t varOffset = 0;
  G4_SrcRegRegion *s0a = NULL;
  // oMask
  G4_SrcRegRegion *oM = NULL;
  if (cntrls.s0aPresent) {
    s0a = msgOpnds[varOffset];
    ++varOffset;
  }
  if (cntrls.oMPresent) {
    oM = msgOpnds[varOffset];
    ++varOffset;
  }

  G4_SrcRegRegion *R = msgOpnds[varOffset++];
  G4_SrcRegRegion *G = msgOpnds[varOffset++];
  G4_SrcRegRegion *B = msgOpnds[varOffset++];
  G4_SrcRegRegion *A = msgOpnds[varOffset++];
  // depth
  G4_SrcRegRegion *Z = NULL;

  if (cntrls.zPresent)
    Z = msgOpnds[varOffset++];

  // stencil
  G4_SrcRegRegion *S = NULL;
  if (cntrls.isStencil) {
    S = msgOpnds[varOffset++];
  }

  if (varOffset != numParms) {
    vASSERT(false);
    return VISA_FAILURE;
  }

  bool FP16Data = R->getType() == Type_HF;
  if (FP16Data) {
    vISA_ASSERT_INPUT((G->isNullReg() || G->getType() == Type_HF) &&
                          (B->isNullReg() || B->getType() == Type_HF) &&
                          (A->isNullReg() || A->getType() == Type_HF),
                      "R,G,B,A for RT write must have the same type");
  }

  auto mult = (execSize == getNativeExecSize() ? 1 : 2);
  mult = (FP16Data) ? 1 : mult;

  // RGBA sr0Alpha take up one GRF in SIMD8 and SIMD16 modes.
  // in SIMD8 upper DWORDs are reserved
  unsigned int numRows = numParms * mult;

  // Depth is always Float
  // For SIMD16 it is 2 grfs
  // For SIMD8  it is 1 grf
  if (FP16Data && cntrls.zPresent && executionSize == EXEC_SIZE_16) {
    ++numRows;
  }

  if (cntrls.oMPresent && mult == 2) {
    // oM is always 1 row irrespective of execSize
    numRows--;
  }

  // although for now HW only supports stencil in SIMD8 mode
  if (cntrls.isStencil && mult == 2) {
    // stencil is always 1 row irrespective of execSize
    numRows--;
  }

  // header is always 64 byte
  const int numDWInHeader = 16;
  const int headerBytes = numDWInHeader * sizeof(int);
  const int numHeaderGRF = numDWInHeader / getNativeExecSize();

  /*
  All other values should be set by default.
  Most of the time when renderTargetIndex != 0, src0Alpha is present also
  */
  bool isRTIdxNonzero = cntrls.RTIndexPresent &&
                        (rtIndex->isSrcRegRegion() ||
                         (rtIndex->isImm() && rtIndex->asImm()->getImm() != 0));
  bool isRTIdxDynamic = cntrls.RTIndexPresent && rtIndex->isSrcRegRegion();
  bool needsHeaderForMRT = isRTIdxDynamic || cntrls.s0aPresent ||
                           (!hasHeaderlessMRTWrite() && isRTIdxNonzero);
  if (needsHeaderForMRT || cntrls.isSampleIndex) {
    useHeader = true;
    numRows += numHeaderGRF;
  }

  bool useSplitSend = useSends();
  // So far, we don't have a obvious cut except for header. As the result,
  // split-send is disabled once there's no header in the message.

  G4_SrcRegRegion *srcToUse = NULL;
  G4_Declare *msg = NULL;
  G4_Declare *msgF = NULL;
  G4_Declare *payloadUD = NULL;
  G4_Declare *payloadUW = NULL;
  G4_Declare *payloadFOrHF = NULL;
  G4_Declare *payloadF = NULL;

  if (useSplitSend) {
    if (useHeader) {
      // subtracting Header
      numRows -= numHeaderGRF;
      // creating header
      msg = createSendPayloadDcl(numDWInHeader, Type_UD);
      msgF = createSendPayloadDcl(numDWInHeader, Type_F);
      msgF->setAliasDeclare(msg, 0);
    }
    // creating payload
    unsigned int numElts = numRows * numEltPerGRF<Type_UB>() / TypeSize(Type_F);
    payloadUD = createSendPayloadDcl(numElts, Type_UD);
    payloadFOrHF = createSendPayloadDcl(numElts, FP16Data ? Type_HF : Type_F);
    payloadUW = createSendPayloadDcl(numElts, Type_UW);
    payloadF = createSendPayloadDcl(numElts, Type_F);

    payloadFOrHF->setAliasDeclare(payloadUD, 0);
    payloadUW->setAliasDeclare(payloadUD, 0);
    payloadF->setAliasDeclare(payloadUD, 0);
  } else {
    unsigned int numElts = numRows * numEltPerGRF<Type_UB>() / TypeSize(Type_F);
    // creating enough space for header + payload
    msg = createSendPayloadDcl(numElts, Type_UD);
    msgF = createSendPayloadDcl(getGenxSamplerIOSize() * 2, Type_F);
    msgF->setAliasDeclare(msg, 0);

    // creating payload declarations.
    payloadUD = createSendPayloadDcl(numElts - (useHeader ? numDWInHeader : 0),
                                     Type_UD);
    payloadFOrHF = createSendPayloadDcl(
        numElts - (useHeader ? numDWInHeader : 0), FP16Data ? Type_HF : Type_F);
    payloadUW = createSendPayloadDcl(numElts - (useHeader ? numDWInHeader : 0),
                                     Type_UW);
    payloadF = createSendPayloadDcl(numElts, Type_F);

    // setting them to alias a top level decl with offset past the header
    payloadUD->setAliasDeclare(msg, useHeader ? headerBytes : 0);
    payloadFOrHF->setAliasDeclare(msg, useHeader ? headerBytes : 0);
    payloadUW->setAliasDeclare(msg, useHeader ? headerBytes : 0);
    payloadF->setAliasDeclare(payloadUD, 0);
  }

  if (useHeader) {
    vISA_ASSERT_INPUT(r1HeaderOpnd,
                      "Second GRF for header that was passed in is NULL.");
    G4_DstRegRegion *payloadRegRgn =
        createDst(msg->getRegVar(), 0, 0, 1, Type_UD);

    G4_Declare *r0 = getBuiltinR0();
    G4_SrcRegRegion *r0RegRgn =
        createSrc(r0->getRegVar(), 0, 0, getRegionStride1(), Type_UD);

    // moves data from r0 to header portion of the message
    G4_INST *movInst =
        createMov(g4::SIMD8, payloadRegRgn, r0RegRgn, InstOpt_NoOpt, true);
    movInst->setOptionOn(InstOpt_WriteEnable);

    payloadRegRgn = createDst(msg->getRegVar(), 1, 0, 1, Type_UD);
    r1HeaderOpnd->setType(*this, Type_UD);
    movInst =
        createMov(g4::SIMD8, payloadRegRgn, r1HeaderOpnd, InstOpt_NoOpt, true);
    movInst->setOptionOn(InstOpt_WriteEnable);

#define SAMPLE_INDEX_OFFSET 6
    if (cntrls.isSampleIndex) {
      G4_Declare *tmpDcl = createTempVar(2, Type_UD, Any);
      G4_DstRegRegion *tmpDst =
          createDst(tmpDcl->getRegVar(), 0, 0, 1, Type_UD);

      createBinOp(G4_shl, g4::SIMD1, tmpDst, sampleIndexOpnd,
                  createImm(SAMPLE_INDEX_OFFSET, Type_UD), InstOpt_WriteEnable,
                  true);

      G4_DstRegRegion *payloadUDRegRgn =
          createDst(msg->getRegVar(), 0, 0, 1, Type_UD);
      G4_SrcRegRegion *tmpSrc =
          createSrc(tmpDcl->getRegVar(), 0, 0, getRegionScalar(), Type_UD);
      G4_SrcRegRegion *payloadSrc =
          createSrc(msg->getRegVar(), 0, 0, getRegionScalar(), Type_UD);
      createBinOp(G4_or, g4::SIMD1, payloadUDRegRgn, payloadSrc, tmpSrc,
                  InstOpt_WriteEnable, true);
    }

    if (isRTIdxNonzero) {
      G4_DstRegRegion *dstRTIRgn =
          createDst(msg->getRegVar(), 0, 2, 1, Type_UD);

      G4_INST *rtiMovInst =
          createMov(g4::SIMD1, dstRTIRgn, rtIndex, InstOpt_NoOpt, true);
      rtiMovInst->setOptionOn(InstOpt_WriteEnable);
    }

    // if header is used, then predication value will need to be stored
    // in the header
    if (useHeader && (pred || cntrls.isHeaderMaskfromCe0)) {
      // moving pixelMask in to payload
      G4_DstRegRegion *dstPixelMaskRgn =
          createDst(msg->getRegVar(), 1, 14, 1, Type_UW);

      // setPixelMaskRgn when WA ce0 is needed
      auto setPixelMaskRgn = [this, dstPixelMaskRgn](G4_InstOption Option) -> void {
        G4_Declare *flagDecl = createTempFlag(2, "WAce0");
        G4_RegVar *flagVar = flagDecl->getRegVar();
        G4_DstRegRegion *flag =
            createDst(flagVar, 0, Option == InstOpt_M16 ? 1 : 0, 1, Type_UW);

        // (1) (W) mov (1|M0) WAce0.[0|1]:uw, 0
        //         M0 : WAce0.0; M16 : WAce0.1
        // (2)     cmp (16|[M0|M16]) (eq)WAce0.0 r0:uw r0:uw
        // (3) (W) mov(1|M0) dstPixelMaskRgn:uw  WAce0.[0|1]:uw
        //         M0 : WAce0.0; M16 : WAce0.1
        createMov(g4::SIMD1, flag, createImm(0, Type_UW), InstOpt_WriteEnable,
                  true);

        G4_SrcRegRegion *r0_0 = createSrc(getRealR0()->getRegVar(), 0, 0,
                                          getRegionStride1(), Type_UW);
        G4_SrcRegRegion *r0_1 = createSrc(getRealR0()->getRegVar(), 0, 0,
                                          getRegionStride1(), Type_UW);
        G4_DstRegRegion *nullDst = createNullDst(Type_UW);
        G4_CondMod *flagCM = createCondMod(Mod_e, flagVar, 0);
        createInst(NULL, G4_cmp, flagCM, g4::NOSAT, g4::SIMD16, nullDst, r0_0,
                   r0_1, Option, true);

        G4_SrcRegRegion *flagSrc =
            createSrc(flagVar, 0, Option == InstOpt_M16 ? 1 : 0,
                      getRegionScalar(), Type_UW);

        // move to dstPixelMaskRgn
        createMov(g4::SIMD1, dstPixelMaskRgn, flagSrc, InstOpt_WriteEnable,
                  true);
      };

      G4_SrcRegRegion *pixelMask = NULL;
      if (emask == vISA_EMASK_M5_NM || emask == vISA_EMASK_M5) {
        if (pred) {
          // this is a Second half of a SIMD32 RT write. We need to get second
          // half of flag register. mov whole register in to GRF, move second
          // word of it in to payload.

          G4_SrcRegRegion *pixelMaskTmp = createSrc(
              pred->getBase()->asRegVar(), 0, 0, getRegionScalar(), Type_UD);
          G4_Declare *tmpDcl = createTempVar(1, Type_UD, Any);
          G4_DstRegRegion *tmpDst =
              createDst(tmpDcl->getRegVar(), 0, 0, 1, Type_UD);
          createMov(g4::SIMD1, tmpDst, pixelMaskTmp, InstOpt_WriteEnable, true);

          pixelMask =
              createSrc(tmpDcl->getRegVar(), 0, 1, getRegionScalar(), Type_UW);

          // move from temp register to header
          createMov(g4::SIMD1, dstPixelMaskRgn, pixelMask, InstOpt_WriteEnable,
                    true);
        } else {
          if (VISA_WA_CHECK(getPWaTable(), Wa_1406950495)) {
            setPixelMaskRgn(InstOpt_M16);
          } else {
            G4_SrcRegRegion *ce0 = createSrc(phyregpool.getMask0Reg(), 0, 0,
                                             getRegionScalar(), Type_UD);

            // shr .14<1>:uw ce0:ud 16:uw
            createBinOp(G4_shr, g4::SIMD1, dstPixelMaskRgn, ce0,
                        createImm(16, Type_UW), InstOpt_WriteEnable, true);
          }
        }
      } else {
        if (pred) {
          pixelMask = createSrc(pred->getBase()->asRegVar(), 0, 0,
                                getRegionScalar(), Type_UW);

          // clearing lower 15 bits
          createMov(g4::SIMD1, dstPixelMaskRgn, pixelMask, InstOpt_WriteEnable,
                    true);
        } else {
          if (VISA_WA_CHECK(getPWaTable(), Wa_1406950495)) {
            setPixelMaskRgn(InstOpt_M0);
          } else {
            G4_SrcRegRegion *ce0 = createSrc(phyregpool.getMask0Reg(), 0, 0,
                                             getRegionScalar(), Type_UD);

            // mov .14<1>:uw ce0:ud.  clearing lower 15 bits
            createMov(g4::SIMD1, dstPixelMaskRgn, ce0, InstOpt_WriteEnable,
                      true);
          }
        }
      }

      pred = NULL;
    }
    unsigned int orImmVal = 0;

    // setting first DWORD of MHC_RT_C0 - Render Target Message Header Control

    if (cntrls.isStencil) {
      orImmVal = (0x1 << 14);
    }

    if (cntrls.zPresent) {
      orImmVal = (0x1 << 13);
    }

    if (cntrls.oMPresent) {
      orImmVal |= (0x1 << 12);
    }

    if (cntrls.s0aPresent) {
      orImmVal |= (0x1 << 11);
    }

    if (orImmVal != 0) {
      G4_SrcRegRegion *immSrcRegRgn =
          createSrc(msg->getRegVar(), 0, 0, getRegionScalar(), Type_UD);

      G4_DstRegRegion *immDstRegRgn =
          createDst(msg->getRegVar(), 0, 0, 1, Type_UD);

      G4_INST *immOrInst =
          createBinOp(G4_or, g4::SIMD1, immDstRegRgn, immSrcRegRgn,
                      createImm(orImmVal, Type_UD), InstOpt_WriteEnable, true);
      immOrInst->setOptionOn(InstOpt_WriteEnable);
    }
  }

  // Check whether coalescing is possible
#define UNINITIALIZED_DWORD 0xffffffff
  unsigned int offset = UNINITIALIZED_DWORD;
  // If the header is not present or split-send is available, we will try to
  // coalesc payload by checking whether the source is already prepared in a
  // continuous region. If so, we could reuse the source region directly
  // instead of copying it again.
  bool canCoalesce = !useHeader || useSplitSend;
  G4_SrcRegRegion *prevRawOpnd = NULL;

  if (R->isNullReg() || G->isNullReg() || B->isNullReg() || A->isNullReg())
    canCoalesce = false;

  if (canCoalesce && cntrls.s0aPresent) {
    prevRawOpnd = s0a;
    offset = getByteOffsetSrcRegion(s0a);
  }

  if (canCoalesce && cntrls.oMPresent) {
    // by default it will check based on first opnd type, but that can be HF, F,
    // we need second operand type according to spec oM is UW
    canCoalesce =
        checkIfRegionsAreConsecutive(prevRawOpnd, oM, execSize, oM->getType());
    prevRawOpnd = oM;
    if (offset == UNINITIALIZED_DWORD) {
      offset = getByteOffsetSrcRegion(oM);
    }
  }

  if (canCoalesce) {
    if (execSize == 16 && cntrls.oMPresent) {
      // oM is 1 GRF for SIMD16 since it is UW type
      canCoalesce = checkIfRegionsAreConsecutive(oM, R, execSize, Type_UW);
      prevRawOpnd = R;
    } else {
      canCoalesce = checkIfRegionsAreConsecutive(prevRawOpnd, R, execSize);
      prevRawOpnd = R;
    }

    if (offset == UNINITIALIZED_DWORD) {
      offset = getByteOffsetSrcRegion(prevRawOpnd);
    }

    if (canCoalesce) {
      auto tempExecSize = execSize;
      if (FP16Data && execSize == 8)
        tempExecSize = g4::SIMD16;
      canCoalesce =
          checkIfRegionsAreConsecutive(prevRawOpnd, G, tempExecSize) &&
          checkIfRegionsAreConsecutive(G, B, tempExecSize) &&
          checkIfRegionsAreConsecutive(B, A, tempExecSize);
      prevRawOpnd = A;
      if (offset == UNINITIALIZED_DWORD) {
        offset = getByteOffsetSrcRegion(A);
        if (FP16Data && execSize == g4::SIMD8)
          offset += 8;
      }
    }
  }

  if (canCoalesce && cntrls.zPresent) {
    canCoalesce = checkIfRegionsAreConsecutive(prevRawOpnd, Z, execSize);
    prevRawOpnd = Z;
  }

  if (canCoalesce && cntrls.isStencil) {
    canCoalesce = checkIfRegionsAreConsecutive(prevRawOpnd, S, execSize);
    prevRawOpnd = S;
  }

  if (canCoalesce == false) {
    // Copy parms to payload
    unsigned regOff = 0;

    if (cntrls.s0aPresent) {

      Copy_SrcRegRegion_To_Payload(payloadFOrHF, regOff, s0a, execSize,
                                   instOpt);
    }

    if (cntrls.oMPresent) {
      Copy_SrcRegRegion_To_Payload(payloadUW, regOff, oM, execSize, instOpt);
      // Copy_SrcRegRegion_To_Payload increments regOff by 1 if byteSize ==2
      // works for oM since in SIMD16 it occupies one GRF
    }

    //   When RT write is HF s0a,R, G, B, A are allowed to be HF.
    //   In SIMD8 upper DWORDS are reserved.
    //   In SIMD16 uppder DOWRDS contain second grf worth of values if type was
    //   F.
    //
    // Output can be only Depth, so V0 is passed in if RGBA don't need to be
    // outputted
    auto offIncrement = 2;
    if (execSize == 8 || FP16Data)
      offIncrement = 1;

    if (!R->isNullReg())
      Copy_SrcRegRegion_To_Payload(payloadFOrHF, regOff, R, execSize, instOpt);
    else
      regOff += offIncrement;

    if (!G->isNullReg())
      Copy_SrcRegRegion_To_Payload(payloadFOrHF, regOff, G, execSize, instOpt);
    else
      regOff += offIncrement;

    if (!B->isNullReg())
      Copy_SrcRegRegion_To_Payload(payloadFOrHF, regOff, B, execSize, instOpt);
    else
      regOff += offIncrement;

    if (!A->isNullReg())
      Copy_SrcRegRegion_To_Payload(payloadFOrHF, regOff, A, execSize, instOpt);
    else
      regOff += offIncrement;

    if (cntrls.zPresent) {
      Copy_SrcRegRegion_To_Payload(payloadF, regOff, Z, execSize, instOpt);
    }

    if (cntrls.isStencil) {
      Copy_SrcRegRegion_To_Payload(payloadFOrHF, regOff, S, execSize,
                                   InstOpt_WriteEnable);
    }

    srcToUse = createSrcRegRegion(payloadUD, getRegionStride1());
  } else {
    // Coalesce and directly use original raw operand
    G4_Declare *dcl = R->getBase()->asRegVar()->getDeclare();
    srcToUse = createSrc(dcl->getRegVar(), offset / 32, 0, getRegionStride1(),
                         R->getType());
  }

  // Now create message message descriptor
  // 7:0 - BTI
  // 10:8 - Render Target Message Subtype
  // 11 - Slot Group Select
  // 12 - Last Render Target Select
  // 13 - Reserved (DevBDW)
  // 13 - Per-Sample PS Outputs Enable (DevSKL+)
  // 17:14 - Message Type
  // 18 - Reserved
  // 19 - Header Present
  // 24:20 - Response Length
  // 28:25 - Message Length
  // 29 - Reserved
  // 30 - Message Precision Subtype (DevBDW+)
  // 31 - Reserved (MBZ)
  unsigned int fc = 0;

  // making explicit
  EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL messageType =
      (executionSize == EXEC_SIZE_8)
          ? EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD8_SINGLE_SOURCE_LOW
          : EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD16_SINGLE_SOURCE;

#define RENDER_TARGET_MESSAGE_SUBTYPE_OFFSET 8
  fc |= (messageType << RENDER_TARGET_MESSAGE_SUBTYPE_OFFSET);

#define SLOT_GROUP_SELECT_OFFSET 11
  // for SIMD32 for second RT Write setting this bit
  if (emask == vISA_EMASK_M5_NM || emask == vISA_EMASK_M5)
    fc |= (0x1 << SLOT_GROUP_SELECT_OFFSET);

  if (cntrls.isLastWrite) {
#define LAST_RENDER_TARGET_SELECT_OFFSET 12
    fc |= (0x1 << LAST_RENDER_TARGET_SELECT_OFFSET);
  }

  if (cntrls.isPerSample) {
#define PER_SAMPLE_PS_ENABLE_OFFSET 13
    fc += (0x1 << PER_SAMPLE_PS_ENABLE_OFFSET);
  }

  if (FP16Data) {
    fc |= 0x1 << MESSAGE_PRECISION_SUBTYPE_OFFSET;
  }

#define MESSAGE_TYPE 14
  fc |= (0xc << MESSAGE_TYPE);

#define COARSE_PIXEL_OUTPUT_ENABLE 18
  if (cntrls.isCoarseMode)
    fc |= 0x1 << COARSE_PIXEL_OUTPUT_ENABLE;
#define CPS_COUNTER_EXT_MSG_DESC_OFFSET 16

  uint16_t extFuncCtrl = 0;
  if (cntrls.isNullRT && getPlatform() >= GENX_TGLLP) {
    // extFuncCtrl is the 16:31 bits of extDesc. NullRT is the bit 20 of
    // extDesc. That says NullRT is the bit 4 of extFuncCtrl.
#define NULL_RENDER_TARGET 4
    extFuncCtrl |= 0x1 << NULL_RENDER_TARGET;
  }

  G4_InstSend *sendInst = nullptr;

  if (useSplitSend || cpsCounter) {
    G4_SendDescRaw *msgDesc = NULL;
    G4_SrcRegRegion *m0 = NULL;
    bool indirectExDesc = false;
    if (useHeader) {
      m0 = createSrcRegRegion(msg, getRegionStride1());
      msgDesc = createSendMsgDesc(fc, 0, numHeaderGRF, SFID::DP_RC, numRows,
                                  extFuncCtrl, SendAccess::WRITE_ONLY, surface);
      msgDesc->setHeaderPresent(useHeader);
    } else {
      if (!isRTIdxNonzero && !cntrls.s0aPresent) {
        // direct imm is a-ok for ext desc
        msgDesc = createSendMsgDesc(fc, 0, numRows, SFID::DP_RC, 0, extFuncCtrl,
                                    SendAccess::WRITE_ONLY, surface);
      } else {
        vISA_ASSERT_INPUT(rtIndex->isImm(), "RTIndex must be imm at this point");
        uint8_t RTIndex = (uint8_t)rtIndex->asImm()->getImm() & 0x7;
        uint32_t desc = G4_SendDescRaw::createDesc(fc, false, numRows, 0);
        uint32_t extDesc = G4_SendDescRaw::createMRTExtDesc(
            cntrls.s0aPresent, RTIndex, false, 0, extFuncCtrl);
        msgDesc = createGeneralMsgDesc(desc, extDesc, SendAccess::WRITE_ONLY,
                                       surface);

        if (!canEncodeFullExtDesc()) {
          // we must use a0 for extended msg desc in this case as there aren't
          // enough bits to encode the full ext desc mov (1) a0.2:ud extDesc
          G4_DstRegRegion *dst = createDstRegRegion(getBuiltinA0Dot2(), 1);
          createMov(g4::SIMD1, dst, createImm(extDesc, Type_UD),
                    InstOpt_WriteEnable, true);
          indirectExDesc = true;
        }
      }
    }

    /*
    If we need to set cps counter then ext_message descriptor
    needs to be a register.
    */
    if (cpsCounter) {
      vISA_ASSERT_INPUT(hasCPS(), "CPS counter is not supported");
      unsigned msgDescValue = msgDesc->getExtendedDesc();

      // shifting CPS counter by appropriate number of bits and storing in
      // ext_descriptor operand
      G4_DstRegRegion *dstMove2 = createDstRegRegion(getBuiltinA0Dot2(), 1);
      G4_Imm *immedOpnd = createImm(msgDescValue, Type_UD);

      /// setting lower bits
      createBinOp(G4_or, g4::SIMD1, dstMove2, cpsCounter, immedOpnd,
                  InstOpt_WriteEnable, true);
      indirectExDesc = true;
    }

    if (!useHeader) {
      m0 = srcToUse;
      srcToUse = createNullSrc(Type_UD);
    }

    sendInst = createSplitSendToRenderTarget(
        pred, createNullDst(Type_UD), m0, srcToUse,
        indirectExDesc
            ? createSrcRegRegion(getBuiltinA0Dot2(), getRegionScalar())
            : nullptr,
        execSize, msgDesc, instOpt);
  } else {
    G4_SrcRegRegion *m = srcToUse;
    if (useHeader)
      m = createSrcRegRegion(msg, getRegionStride1());
    sendInst = createSendInst(
        pred, createNullDst(Type_UD), m, numRows, 0, execSize, fc, SFID::DP_RC,
        useHeader, SendAccess::WRITE_ONLY, surface, NULL, instOpt, true);
  }
  if (getOption(vISA_renderTargetWriteSendReloc)) {
    std::string symbolName{"RTW_SEND"};
    RelocationEntry::createRelocation(kernel, *sendInst, 0, symbolName,
                                      GenRelocType::R_SEND);
  }
  return VISA_SUCCESS;
}

/*****************************************************************************\
ENUM: EU_PVC_DATA_PORT_RENDER_TARGET_WRITE_SUBTYPE
\*****************************************************************************/
enum EU_XE2_DATA_PORT_RENDER_TARGET_WRITE_SUBTYPE {
  EU_XE2_DATA_PORT_RENDER_TARGET_WRITE_SUBTYPE_SIMD16_SINGLE_SOURCE = 0,
  EU_XE2_DATA_PORT_RENDER_TARGET_WRITE_SUBTYPE_SIMD32 = 1,
  EU_XE2_DATA_PORT_RENDER_TARGET_WRITE_SUBTYPE_SIMD16_DUAL_SOURCE = 2
};

// TODO: combine this with constructSrcPayloadDualRenderTarget function
std::tuple<G4_SrcRegRegion *, uint32_t, uint32_t>
IR_Builder::constructSrcPayloadDualRenderTarget(vISA_RT_CONTROLS cntrls,
                                                G4_SrcRegRegion **msgOpnds,
                                                unsigned int numMsgOpnds,
                                                G4_ExecSize execSize,
                                                G4_InstOpts instOpt) {
  uint8_t varOffset = 0;
  G4_SrcRegRegion *oM = nullptr;
  if (cntrls.oMPresent) {
    oM = msgOpnds[varOffset];
    ++varOffset;
  }

  G4_SrcRegRegion *s0R = msgOpnds[varOffset++];
  G4_SrcRegRegion *s0G = msgOpnds[varOffset++];
  G4_SrcRegRegion *s0B = msgOpnds[varOffset++];
  G4_SrcRegRegion *s0A = msgOpnds[varOffset++];
  G4_SrcRegRegion *s1R = msgOpnds[varOffset++];
  G4_SrcRegRegion *s1G = msgOpnds[varOffset++];
  G4_SrcRegRegion *s1B = msgOpnds[varOffset++];
  G4_SrcRegRegion *s1A = msgOpnds[varOffset++];

  // depth
  G4_SrcRegRegion *Z = nullptr;
  if (cntrls.zPresent)
    Z = msgOpnds[varOffset++];

  // stencil
  G4_SrcRegRegion *S = nullptr;
  if (cntrls.isStencil) {
    S = msgOpnds[varOffset++];
  }

  if (varOffset != numMsgOpnds) {
    vASSERT(false);
    return std::make_tuple(nullptr, 0, 0);
  }

  auto checkType = [](G4_SrcRegRegion *src) {
    return src->getType() == Type_F || src->isNullReg();
  };
  vISA_ASSERT_INPUT(checkType(s0R) && checkType(s0G) && checkType(s0B) && checkType(s0A) &&
         checkType(s1R) && checkType(s1G) && checkType(s1B) && checkType(s1A),
         "RGBA type must be F");

  // compute payload size sans header
  uint32_t numRows = 8; // s0R, s0G, s0B, s0A, s1R, s1G, s1B, s1A

  if (cntrls.zPresent) {
    // Depth is always Float
    numRows++;
  }

  if (cntrls.oMPresent) {
    numRows++;
  }

  if (cntrls.isStencil) {
    numRows++;
  }

  // creating payload
  unsigned int numElts = numRows * getGRFSize() / TypeSize(Type_F);
  auto payloadUD = createSendPayloadDcl(numElts, Type_UD);
  auto payloadUW = createSendPayloadDcl(numElts, Type_UW);
  auto payloadF = createSendPayloadDcl(numElts, Type_F);
  auto payloadUB = createSendPayloadDcl(numElts, Type_UB);

  payloadUW->setAliasDeclare(payloadUD, 0);
  payloadF->setAliasDeclare(payloadUD, 0);
  payloadUB->setAliasDeclare(payloadUD, 0);

  // Check whether coalescing is possible
  // coalesc payload by checking whether the source is already prepared in a
  // continuous region. If so, we could reuse the source region directly
  // instead of copying it again.
  bool canCoalesce = true;
  G4_SrcRegRegion *leadingParam = cntrls.oMPresent ? oM : s0R;

  if (s0R->isNullReg() || s0G->isNullReg() || s0B->isNullReg() ||
      s0A->isNullReg() || s1R->isNullReg() || s1G->isNullReg() ||
      s1B->isNullReg() || s1A->isNullReg()) {
    canCoalesce = false;
  }

  if (canCoalesce) {
    auto payloadDcl = leadingParam->getTopDcl()->getRootDeclare();
    uint32_t nextOffset = getByteOffsetSrcRegion(leadingParam);

    // oM is leading param if present, so no need to check for its
    // contiguousness
    auto isContiguous = [this](G4_SrcRegRegion *src, uint32_t offset,
                               G4_Declare *dcl) {
      auto srcDcl = src->getTopDcl()->getRootDeclare();
      if (srcDcl != dcl) {
        return false; // different declares are not contiguous
      }
      return offset ==
             getByteOffsetSrcRegion(
                 src); // offset must be equal to the src's byte offset
    };

    if (canCoalesce) {
      canCoalesce = isContiguous(s0R, nextOffset, payloadDcl);
      nextOffset += getGRFSize();
      if (canCoalesce) {
        canCoalesce = isContiguous(s0G, nextOffset, payloadDcl);
        nextOffset += getGRFSize();
      }
      if (canCoalesce) {
        canCoalesce = isContiguous(s0B, nextOffset, payloadDcl);
        nextOffset += getGRFSize();
      }
      if (canCoalesce) {
        canCoalesce = isContiguous(s0A, nextOffset, payloadDcl);
        nextOffset += getGRFSize();
      }
      if (canCoalesce) {
        canCoalesce = isContiguous(s1R, nextOffset, payloadDcl);
        nextOffset += getGRFSize();
      }
      if (canCoalesce) {
        canCoalesce = isContiguous(s1G, nextOffset, payloadDcl);
        nextOffset += getGRFSize();
      }
      if (canCoalesce) {
        canCoalesce = isContiguous(s1B, nextOffset, payloadDcl);
        nextOffset += getGRFSize();
      }
      if (canCoalesce) {
        canCoalesce = isContiguous(s1A, nextOffset, payloadDcl);
        nextOffset += getGRFSize();
      }
    }

    if (canCoalesce && cntrls.zPresent) {
      canCoalesce = isContiguous(Z, nextOffset, payloadDcl);
      nextOffset += getGRFSize();
    }

    // last element is stencil
    if (canCoalesce && cntrls.isStencil) {
      canCoalesce = isContiguous(S, nextOffset, payloadDcl);
    }
  }

  G4_SrcRegRegion *srcToUse = nullptr;
  if (!canCoalesce) {
    // Copy parameters to payload
    // ToDo: optimize to generate split send
    unsigned regOff = 0;

    if (cntrls.oMPresent) {
      Copy_SrcRegRegion_To_Payload(payloadUW, regOff, oM, execSize, instOpt);
    }

    if (!s0R->isNullReg())
      Copy_SrcRegRegion_To_Payload(payloadF, regOff, s0R, execSize, instOpt);
    else
      regOff++;

    if (!s0G->isNullReg())
      Copy_SrcRegRegion_To_Payload(payloadF, regOff, s0G, execSize, instOpt);
    else
      regOff++;

    if (!s0B->isNullReg())
      Copy_SrcRegRegion_To_Payload(payloadF, regOff, s0B, execSize, instOpt);
    else
      regOff++;

    if (!s0A->isNullReg())
      Copy_SrcRegRegion_To_Payload(payloadF, regOff, s0A, execSize, instOpt);
    else
      regOff++;
    if (!s1R->isNullReg())
      Copy_SrcRegRegion_To_Payload(payloadF, regOff, s1R, execSize, instOpt);
    else
      regOff++;

    if (!s1G->isNullReg())
      Copy_SrcRegRegion_To_Payload(payloadF, regOff, s1G, execSize, instOpt);
    else
      regOff++;

    if (!s1B->isNullReg())
      Copy_SrcRegRegion_To_Payload(payloadF, regOff, s1B, execSize, instOpt);
    else
      regOff++;

    if (!s1A->isNullReg())
      Copy_SrcRegRegion_To_Payload(payloadF, regOff, s1A, execSize, instOpt);
    else
      regOff++;

    if (cntrls.zPresent) {
      Copy_SrcRegRegion_To_Payload(payloadF, regOff, Z, execSize, instOpt);
    }

    if (cntrls.isStencil) {
      Copy_SrcRegRegion_To_Payload(payloadUB, regOff, S, execSize, instOpt);
    }

    srcToUse = createSrcRegRegion(payloadUD, getRegionStride1());
  } else {
    // Coalesce and directly use original raw operand
    leadingParam->setType(*this,
                          s0R->getType()); // it shouldn't matter, but change it
                                           // in case leading param is oM
    srcToUse = leadingParam;
  }
  // set chmask
  // TODO: is chmask based on src0 or src1? For now assuming it is based on
  // src0 or src1
  uint32_t chMask =
      (!(s0R->isNullReg() && s1R->isNullReg()) ? 0x1 : 0) |
      ((!(s0G->isNullReg() && s1G->isNullReg()) ? 0x1 : 0) << 0x1) |
      ((!(s0B->isNullReg() && s1B->isNullReg()) ? 0x1 : 0) << 0x2) |
      (((!(s0A->isNullReg() && s1A->isNullReg()) || cntrls.s0aPresent) ? 0x1 : 0) << 0x3);


  return std::make_tuple(srcToUse, numRows, chMask);
}

std::tuple<G4_SrcRegRegion *, uint32_t, uint32_t>
IR_Builder::constructSrcPayloadRenderTarget(vISA_RT_CONTROLS cntrls,
                                            G4_SrcRegRegion **msgOpnds,
                                            unsigned int numMsgOpnds,
                                            G4_ExecSize execSize,
                                            G4_InstOpts instOpt) {
  uint8_t varOffset = 0;
  G4_SrcRegRegion *s0a = nullptr;
  G4_SrcRegRegion *oM = nullptr;
  if (cntrls.s0aPresent) {
    s0a = msgOpnds[varOffset];
    ++varOffset;
  }
  if (cntrls.oMPresent) {
    oM = msgOpnds[varOffset];
    ++varOffset;
  }

  G4_SrcRegRegion *R = msgOpnds[varOffset++];
  G4_SrcRegRegion *G = msgOpnds[varOffset++];
  G4_SrcRegRegion *B = msgOpnds[varOffset++];
  G4_SrcRegRegion *A = msgOpnds[varOffset++];
  // depth
  G4_SrcRegRegion *Z = nullptr;

  if (cntrls.zPresent)
    Z = msgOpnds[varOffset++];

  // stencil
  G4_SrcRegRegion *S = nullptr;
  if (cntrls.isStencil) {
    S = msgOpnds[varOffset++];
  }

  if (varOffset != numMsgOpnds) {
    vASSERT(false);
    return std::make_tuple(nullptr, 0, 0);
  }

  auto checkType = [](G4_SrcRegRegion *src) {
    return src == nullptr || src->getType() == Type_F || src->isNullReg();
  };
  vISA_ASSERT_INPUT(checkType(R) && checkType(G) && checkType(B) && checkType(A),
         "RGBA type must be F");

  // mult is the size of s0a, R, G, B, A, Z(1 or 2 GRF)
  auto mult = (execSize == getNativeExecSize() ? 1 : 2);

  // compute payload size sans header

  uint32_t numRows = 4 * mult; // RGBA

  if (cntrls.s0aPresent) {
    // s0a has same type as RGBA
    numRows += mult;
  }

  if (cntrls.zPresent) {
    // Depth is always Float
    numRows += mult;
  }

  if (cntrls.oMPresent) {
    numRows++;
  }

  if (cntrls.isStencil) {
    numRows++;
  }

  G4_SrcRegRegion *srcToUse = nullptr;

  if (numRows > 0)
  {
    // creating payload
    unsigned int numElts = numRows * getGRFSize() / TypeSize(Type_F);
    auto payloadUD = createSendPayloadDcl(numElts, Type_UD);
    auto payloadUW = createSendPayloadDcl(numElts, Type_UW);
    auto payloadF = createSendPayloadDcl(numElts, Type_F);
    auto payloadUB = createSendPayloadDcl(numElts, Type_UB);

    payloadUW->setAliasDeclare(payloadUD, 0);
    payloadF->setAliasDeclare(payloadUD, 0);
    payloadUB->setAliasDeclare(payloadUD, 0);

    // Check whether coalescing is possible
    // coalesc payload by checking whether the source is already prepared in a
    // continuous region. If so, we could reuse the source region directly
    // instead of copying it again.
    bool canCoalesce = true;
    G4_SrcRegRegion *leadingParam =
        cntrls.s0aPresent ? s0a : (cntrls.oMPresent ? oM : R);

    if (R == nullptr || R->isNullReg() ||
        G == nullptr || G->isNullReg() ||
        B == nullptr || B->isNullReg() ||
        A == nullptr || A->isNullReg()) {
      canCoalesce = false;
    }

    if (canCoalesce) {
      auto payloadDcl = leadingParam->getTopDcl()->getRootDeclare();
      uint32_t nextOffset = getByteOffsetSrcRegion(leadingParam);

      // s0a is leading param if present, so no need to check for its
      // contiguousness
      auto isContiguous = [this](G4_SrcRegRegion *src, uint32_t offset,
                                 G4_Declare *dcl) {
        auto srcDcl = src->getTopDcl()->getRootDeclare();
        if (srcDcl != dcl) {
          return false; // different declares are not contiguous
        }
        return offset ==
               getByteOffsetSrcRegion(
                   src); // offset must be equal to the src's byte offset
      };

      if (canCoalesce && cntrls.oMPresent) {
        canCoalesce = isContiguous(oM, nextOffset, payloadDcl);
        nextOffset += getGRFSize();
      }

      if (canCoalesce) {
        canCoalesce = isContiguous(R, nextOffset, payloadDcl);
        nextOffset += getGRFSize() * mult;
        if (canCoalesce) {
          canCoalesce = isContiguous(G, nextOffset, payloadDcl);
          nextOffset += getGRFSize() * mult;
        }
        if (canCoalesce) {
          canCoalesce = isContiguous(B, nextOffset, payloadDcl);
          nextOffset += getGRFSize() * mult;
        }
        if (canCoalesce) {
          canCoalesce = isContiguous(A, nextOffset, payloadDcl);
          nextOffset += getGRFSize() * mult;
        }
      }

      if (canCoalesce && cntrls.zPresent) {
        canCoalesce = isContiguous(Z, nextOffset, payloadDcl);
        nextOffset += getGRFSize() * mult;
      }

      // last element is stencil
      if (canCoalesce && cntrls.isStencil) {
        canCoalesce = isContiguous(S, nextOffset, payloadDcl);
      }
    }

    if (!canCoalesce) {
      // Copy parameters to payload
      // ToDo: optimize to generate split send
      unsigned regOff = 0;

      if (cntrls.s0aPresent) {
        Copy_SrcRegRegion_To_Payload(payloadF, regOff, s0a, execSize, instOpt);
      }

      if (cntrls.oMPresent) {
        Copy_SrcRegRegion_To_Payload(payloadUW, regOff, oM, execSize, instOpt);
      }

      auto offIncrement = mult;

      if (R != nullptr) {
        if (!R->isNullReg())
          Copy_SrcRegRegion_To_Payload(payloadF, regOff, R, execSize, instOpt);
        else
          regOff += offIncrement;
      }

      if (G != nullptr) {
        if (!G->isNullReg())
          Copy_SrcRegRegion_To_Payload(payloadF, regOff, G, execSize, instOpt);
        else
          regOff += offIncrement;
      }

      if (B != nullptr) {
        if (!B->isNullReg())
          Copy_SrcRegRegion_To_Payload(payloadF, regOff, B, execSize, instOpt);
        else
          regOff += offIncrement;
      }

      if (A != nullptr) {
        if (!A->isNullReg())
          Copy_SrcRegRegion_To_Payload(payloadF, regOff, A, execSize, instOpt);
        else
          regOff += offIncrement;
      }

      if (cntrls.zPresent) {
        Copy_SrcRegRegion_To_Payload(payloadF, regOff, Z, execSize, instOpt);
      }

      if (cntrls.isStencil) {
        Copy_SrcRegRegion_To_Payload(payloadUB, regOff, S, execSize, instOpt);
      }

      srcToUse = createSrcRegRegion(payloadUD, getRegionStride1());
    } else {
      // Coalesce and directly use original raw operand
      leadingParam->setType(*this,
                            R->getType()); // it shouldn't matter, but change it
                                           // in case leading param is oM
      srcToUse = leadingParam;
    }
  } else {
      numRows = 1;
      srcToUse = createNullSrc(Type_UD);
  }
  // set chmask
  uint32_t chMask = (R && !R->isNullReg() ? 0x1 : 0) |
                    ((G && !G->isNullReg() ? 0x1 : 0) << 0x1) |
                    ((B && !B->isNullReg() ? 0x1 : 0) << 0x2) |
                    ((A && !A->isNullReg() ? 0x1 : 0) << 0x3);


  return std::make_tuple(srcToUse, numRows, chMask);
}



// Bit 15 of aoffimmi is set in messages with sampler index >= 16.
static bool IsSamplerIndexGE16(G4_Operand *aoffimmi) {
  bool ret = false;
  if (aoffimmi && aoffimmi->isImm()) {
    const uint16_t aoffimmiVal = (uint16_t)aoffimmi->asImm()->getInt();
    ret = (aoffimmiVal & 0x8000) != 0;
  }
  return ret;
}

// return the contents of M0.2 for sampler messages.  It must be an immediate
// value
static uint32_t createSampleHeader0Dot2(VISASampler3DSubOpCode op,
                                        bool pixelNullMask, uint16_t aoffimmi,
                                        ChannelMask channels,
                                        IR_Builder *builder) {
  uint32_t secondDword = aoffimmi & 0xfff;
  switch (op) {
  case VISA_3D_GATHER4:
  case VISA_3D_GATHER4_L:
  case VISA_3D_GATHER4_B:
  case VISA_3D_GATHER4_I:
  case VISA_3D_GATHER4_PO_PACKED:
  case VISA_3D_GATHER4_PO_PACKED_L:
  case VISA_3D_GATHER4_PO_PACKED_B:
  case VISA_3D_GATHER4_PO_PACKED_I:
       // gather4 source channel select
    secondDword |= (channels.getSingleChannel() << 16);
    break;
  case VISA_3D_GATHER4_PO:
    if (builder->hasGather4PO()) {
      secondDword |= (channels.getSingleChannel() << 16);
    }
    static_assert(VISA_3D_SAMPLE_D_C_MLOD == VISA_3D_GATHER4_PO,
                  "Code below needs update");
    if (builder->hasSampleMlod()) {
      vASSERT(!builder->hasGather4PO());
      // RGBA write channel mask
      secondDword |= (channels.getHWEncoding() << 12);
    }
    break;
  case VISA_3D_GATHER4_PO_C:
    static_assert(VISA_3D_SAMPLE_MLOD == VISA_3D_GATHER4_PO_C,
                  "Code below needs update");
    if (builder->hasSampleMlod()) {
      vASSERT(!builder->hasGather4PO());
      // RGBA write channel mask
      secondDword |= (channels.getHWEncoding() << 12);
    }
    break;
  case VISA_3D_GATHER4_C:
  case VISA_3D_GATHER4_I_C:
  case VISA_3D_GATHER4_L_C:
  case VISA_3D_GATHER4_PO_PACKED_C:
  case VISA_3D_GATHER4_PO_PACKED_I_C:
  case VISA_3D_GATHER4_PO_PACKED_L_C:
       // do nothing as channle must be Red (0)
    break;
  default:
    // RGBA write channel mask
    secondDword |= (channels.getHWEncoding() << 12);
    break;
  }

  // M0.2:23, Pixel Null Mask Enable.
  // Only valid for SKL+, and ignored otherwise.
  if (builder->hasPixelNullMask() && pixelNullMask) {
    secondDword |= 1 << 23;
  }

  if (builder->getOption(vISA_EnableProgrammableOffsetsMessageBitInHeader)) {
    // M0.2:24 message type encoding bit 6
    if (op > 31) {
      secondDword |= 1 << 24;
    }
  }
  return secondDword;
}

//
// Coarse Pixel Shading(CPS) LOD compensation enable.
//
// - must be disabled if the response length of the message is zero;
// - must be disabled if the messages is from a 32-pixel dispatch thread;
// - must be disabled unless SIMD Mode is SIMD8* or SIMD16*;
// - only available for sample, sample_b, sample_bc, sample_c, and LOD.
//
static void checkCPSEnable(VISASampler3DSubOpCode op, unsigned reponseLength,
                           unsigned execSize) {

  vISA_ASSERT_INPUT(reponseLength > 0,
                    "CPS LOD Compensation Enable must be disabled if the "
                    "response length is zero");

  vISA_ASSERT_INPUT(execSize == 8 || execSize == 16,
      "CPS LOD Compensation Enable only valid for SIMD8* or SIMD16*");

  [[maybe_unused]]
  bool isCPSAvailable = op == VISA_3D_SAMPLE || op == VISA_3D_SAMPLE_B ||
                        op == VISA_3D_SAMPLE_C || op == VISA_3D_SAMPLE_B_C ||
                        op == VISA_3D_SAMPLE_PO || op == VISA_3D_SAMPLE_PO_B ||
                        op == VISA_3D_SAMPLE_PO_C ||
                        op == VISA_3D_LOD;

  vISA_ASSERT(isCPSAvailable, "CPD LOD Compensation Enable only available for "
                              "sample, sample_b, sample_bc, sample_c and LOD");
}

static G4_Operand *createSampleHeader(IR_Builder *builder, G4_Declare *header,
                                      VISASampler3DSubOpCode actualop,
                                      bool pixelNullMask, G4_Operand *aoffimmi,
                                      ChannelMask srcChannel,
                                      G4_Operand *pairedResource,
                                      G4_Operand *sampler) {
  G4_Operand *retSampler = sampler;
  uint16_t aoffimmiVal =
      aoffimmi->isImm() ? (uint16_t)aoffimmi->asImm()->getInt() : 0;

  unsigned int secondDword = createSampleHeader0Dot2(
      actualop, pixelNullMask, aoffimmiVal, srcChannel, builder);


  G4_Imm *immOpndSecondDword = builder->createImm(secondDword, Type_UD);
  G4_DstRegRegion *payloadDstRgn =
      builder->createDst(header->getRegVar(), 0, 2, 1, Type_UD);
  [[maybe_unused]] G4_INST *headerInst = nullptr;
  if (aoffimmi->isImm()) {
    // mov (1) payload(0,2) immOpndSecondDword
    headerInst =
        builder->createMov(g4::SIMD1, payloadDstRgn, immOpndSecondDword,
                           InstOpt_WriteEnable, true);
  } else {
    // or (1) payload(0,2) aoffimmi<0;1,0>:uw immOpndSeconDword
    headerInst =
        builder->createBinOp(G4_or, g4::SIMD1, payloadDstRgn, aoffimmi,
                             immOpndSecondDword, InstOpt_WriteEnable, true);
  }

  if (sampler != nullptr) {
    builder->doSamplerHeaderMove(header, sampler);

    // Use bit 15 of aoffimmi to tell VISA the sample index could be greater
    // than 15.  In this case, we need to use msg header, and setup M0.3
    // to point to next 16 sampler state.
    if (IsSamplerIndexGE16(aoffimmi)) {
      retSampler = builder->emitSampleIndexGE16(sampler, header);
    }
  }

  builder->doPairedResourceHeaderMove(header, pairedResource);
  return retSampler;
}


static bool needsNoMaskCoordinates(VISASampler3DSubOpCode opcode) {
  return opcode == VISA_3D_SAMPLE || opcode == VISA_3D_SAMPLE_B ||
         opcode == VISA_3D_SAMPLE_C ||
         opcode == VISA_3D_SAMPLE_MLOD || opcode == VISA_3D_SAMPLE_C_MLOD ||
         opcode == VISA_3D_GATHER4_B || opcode == VISA_3D_GATHER4_I ||
         opcode == VISA_3D_GATHER4_I_C || opcode == VISA_3D_SAMPLE_PO ||
         opcode == VISA_3D_SAMPLE_PO_B || opcode == VISA_3D_SAMPLE_PO_C ||
         opcode == VISA_3D_GATHER4_PO_PACKED_B ||
         opcode == VISA_3D_GATHER4_PO_PACKED_I ||
         opcode == VISA_3D_GATHER4_PO_PACKED_I_C ||
         opcode == VISA_3D_SAMPLE_B_C || opcode == VISA_3D_LOD ||
         opcode == VISA_3D_SAMPLE_KILLPIX;
}

static uint8_t getUPosition(VISASampler3DSubOpCode opcode) {
  uint8_t position = 0;
  switch (opcode) {
  case VISA_3D_SAMPLE:
  case VISA_3D_LOD:
  case VISA_3D_SAMPLE_D:
  case VISA_3D_SAMPLE_LZ:
  case VISA_3D_SAMPLE_KILLPIX:
  case VISA_3D_GATHER4_I:
  case VISA_3D_SAMPLE_PO:
  case VISA_3D_SAMPLE_PO_D:
  case VISA_3D_GATHER4_PO_PACKED_I:
    position = 0;
    break;
  case VISA_3D_SAMPLE_B:
  case VISA_3D_SAMPLE_L:
  case VISA_3D_SAMPLE_C:
  case VISA_3D_SAMPLE_D_C:
  case VISA_3D_SAMPLE_C_LZ:
  case VISA_3D_SAMPLE_D_C_MLOD:
  case VISA_3D_SAMPLE_MLOD:
  case VISA_3D_GATHER4_B:
  case VISA_3D_GATHER4_I_C:
  case VISA_3D_SAMPLE_PO_B:
  case VISA_3D_SAMPLE_PO_L:
  case VISA_3D_SAMPLE_PO_C:
  case VISA_3D_GATHER4_PO_PACKED_B:
  case VISA_3D_GATHER4_PO_PACKED_I_C:
    position = 1;
    break;
  case VISA_3D_SAMPLE_B_C:
  case VISA_3D_SAMPLE_L_C:
  case VISA_3D_SAMPLE_C_MLOD:
  case VISA_3D_SAMPLE_PO_L_C:
    position = 2;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("unexpected sampler operation");
    return 0;
  }
  return position;
}

static void setUniformSampler(G4_InstSend *sendInst, bool uniformSampler) {
  if (!uniformSampler) {
    sendInst->setSerialize();
  }
}

/*
Need to split sample_d and sample_dc in to two simd8 sends since HW doesn't
support it. Also need to split any sample instruciton that has more then 5
parameters. Since there is a limit on msg length.
*/
static unsigned TmpSmplDstID = 0;

// split simd32/16 sampler messages into simd16/8 messages due to HW limitation.
int IR_Builder::splitSampleInst(
    VISASampler3DSubOpCode actualop, bool pixelNullMask, bool cpsEnable,
    G4_Predicate *pred, ChannelMask srcChannel, int numChannels,
    G4_Operand *aoffimmi, G4_Operand *sampler, G4_Operand *surface,
    G4_Operand *pairedResource,
    G4_DstRegRegion *dst, VISA_EMask_Ctrl emask, bool useHeader,
    unsigned numRows, // msg length for each simd8
    unsigned int numParms, G4_SrcRegRegion **params, bool uniformSampler) {
  int status = VISA_SUCCESS;
  G4_SrcRegRegion *secondHalf[12];

  bool isHalfReturn = dst->getTypeSize() == 2;
  const bool halfInput = params[0]->getTypeSize() == 2;

  // Now, depending on message type emit out parms to payload
  unsigned regOff = (useHeader ? 1 : 0);
  G4_SrcRegRegion *temp = nullptr;
  G4_ExecSize execSize = getNativeExecSize();
  uint16_t numElts = numRows * numEltPerGRF<Type_F>();
  G4_Declare *payloadF = createSendPayloadDcl(numElts, Type_F);
  G4_Declare *payloadUD = createTempVar(numElts, Type_UD, getGRFAlign());
  payloadUD->setAliasDeclare(payloadF, 0);
  G4_SrcRegRegion *srcToUse =
      createSrc(payloadUD->getRegVar(), 0, 0, getRegionStride1(), Type_UD);

  // even though we only use lower half of the GRF, we have to allocate full GRF
  G4_Declare *payloadHF = createTempVar(numElts * 2, Type_HF, Any);
  payloadHF->setAliasDeclare(payloadF, 0);

  /********* Creating temp destination, since results are interleaved
   * **************/
  G4_DstRegRegion *dst1 = createNullDst(dst->getType());
  G4_Declare *originalDstDcl = nullptr;
  G4_Declare *tempDstDcl = nullptr;
  bool pixelNullMaskEnable = false;
  unsigned tmpDstRows = 0;
  if (!dst->isNullReg()) {
    originalDstDcl = dst->getBase()->asRegVar()->getDeclare();
    tmpDstRows = numChannels;

    // If Pixel Null Mask is enabled, then one extra GRF is needed for the
    // write back message.
    pixelNullMaskEnable = hasPixelNullMask() && pixelNullMask;
    if (pixelNullMaskEnable) {
      vISA_ASSERT_INPUT(useHeader, "pixel null mask requires a header");
      ++tmpDstRows;
    }

    const char *name = getNameString(20, "%s%d", "TmpSmplDst_", TmpSmplDstID++);

    tempDstDcl = createDeclare(
        name, originalDstDcl->getRegFile(), originalDstDcl->getNumElems(),
        (uint16_t)tmpDstRows, originalDstDcl->getElemType());

    dst1 = createDstRegRegion(dst->getRegAccess(), tempDstDcl->getRegVar(), 0,
                              0, 1, dst->getType());
  }
  /********* End creating temp destination ***********************/

  G4_Declare *header = nullptr;

  if (useHeader) {
    const bool samplerIndexGE16 = IsSamplerIndexGE16(aoffimmi);
    bool bindlessSampler = sampler ? isBindlessSampler(sampler) : false;
    header = getSamplerHeader(bindlessSampler, samplerIndexGE16);
    sampler = createSampleHeader(this, header, actualop, pixelNullMask,
                                 aoffimmi, srcChannel,
                                 pairedResource,
                                 sampler);
    createMovInst(payloadUD, 0, 0, g4::SIMD8, nullptr, nullptr,
                  createSrcRegRegion(header, getRegionStride1()), true);
  }

  G4_InstOpts instOpt = Get_Gen4_Emask(emask, execSize);
  for (unsigned paramCounter = 0; paramCounter < numParms; ++paramCounter) {
    temp = params[paramCounter];
    uint32_t MovInstOpt = InstOpt_WriteEnable;
    if (temp->getTypeSize() == 2) {
      // we should generate
      // mov (8) dst<1>:hf src.0<8;8,1>:hf
      G4_DstRegRegion *dstHF =
          createDst(payloadHF->getRegVar(), regOff++, 0, 1, temp->getType());
      temp->setRegion(*this, getRegionStride1());
      createMov(g4::SIMD8, dstHF, temp, MovInstOpt, true);
    } else {
      Copy_SrcRegRegion_To_Payload(payloadF, regOff, temp, execSize,
                                   MovInstOpt);
    }
  }

  uint32_t responseLength =
      getSamplerResponseLength(numChannels, isHalfReturn, execSize,
                               pixelNullMaskEnable, dst->isNullReg());

  uint32_t fc = createSamplerMsgDesc(actualop, execSize == getNativeExecSize(),
                                     isHalfReturn, halfInput);
  uint32_t desc =
      G4_SendDescRaw::createDesc(fc, useHeader, numRows, responseLength);

  if (cpsEnable) {
    checkCPSEnable(actualop, responseLength, 8);
  }
  G4_SendDescRaw *msgDesc =
      createSampleMsgDesc(desc, cpsEnable, 0, surface, sampler);

  G4_InstSend *sendInst = nullptr;
  bool forceSplitSend = shouldForceSplitSend(surface);

  if (forceSplitSend) {
    sendInst = createSplitSendInst(pred, dst1, srcToUse, createNullSrc(Type_UD),
                                   execSize, msgDesc, instOpt, false);
  } else {
    sendInst =
        createSendInst(pred, dst1, srcToUse, execSize, msgDesc, instOpt, false);
  }
  setUniformSampler(sendInst, uniformSampler);

  // SKL+
  // For SIMD8
  //
  // W4.7:1 Reserved (not written): This W4 is only delivered when Pixel Null
  //        Mask Enable is enabled.
  //
  // W4.0  32:8 Reserved: always written as 0xffffff
  //        7:0 Pixel Null Mask: This field has the bit for all pixels set
  //            to 1 except those pixels in which a null page was source for
  //            at least one texel.
  //
  // Need to combine the results from the above two writewback messages.
  // Denote by U0[W4:0] the last row of the first writeback message, and
  // by U1[W4:0] the last row of the second writeback message. Then the last
  // row of the whole writeback message is to take the bitwise OR of
  // U0[W4:0] and U1[W4:0].
  G4_Declare *tempDstUD = 0;
  G4_Declare *tempDst2UD = 0;
  G4_Declare *origDstUD = 0;

  // temp dst for the second send
  G4_DstRegRegion *dst2 = createNullDst(dst->getType());
  G4_Declare *tempDstDcl2 = nullptr;
  if (!dst->isNullReg()) {
    const char *name =
        getNameString(20, "%s%d", "TmpSmplDst2_", TmpSmplDstID++);

    tempDstDcl2 = createDeclare(
        name, originalDstDcl->getRegFile(), originalDstDcl->getNumElems(),
        (uint16_t)tmpDstRows, originalDstDcl->getElemType());

    if (pixelNullMaskEnable) {
      unsigned int numElts =
          tempDstDcl->getNumElems() * tempDstDcl->getNumRows();
      tempDstUD = createTempVar(numElts, Type_UD, getGRFAlign());
      tempDstUD->setAliasDeclare(tempDstDcl, 0);

      numElts = tempDstDcl2->getNumElems() * tempDstDcl2->getNumRows();
      tempDst2UD = createTempVar(numElts, Type_UD, getGRFAlign());
      tempDst2UD->setAliasDeclare(tempDstDcl2, 0);

      numElts = originalDstDcl->getNumElems() * originalDstDcl->getNumRows();
      origDstUD = createTempVar(numElts, Type_UD, getGRFAlign());
      origDstUD->setAliasDeclare(originalDstDcl, 0);
    }

    dst2 = createDstRegRegion(dst->getRegAccess(), tempDstDcl2->getRegVar(), 0,
                              0, 1, dst->getType());
  }
  // update emask
  emask = Get_Next_EMask(emask, execSize);
  G4_InstOpts instOpt2 = Get_Gen4_Emask(emask, execSize);

  auto dupPredicate = [this](G4_Predicate *pred) {
    G4_Predicate *pred2 = nullptr;
    if (pred) {
      pred2 = createPredicate(pred->getState(), pred->getBase(), 0);
    }

    return pred2;
  };

  {
    /**************** SECOND HALF OF THE SEND *********************/
    // re-create payload declare so the two sends may be issued independently
    G4_Declare *payloadF = createSendPayloadDcl(numElts, Type_F);
    G4_Declare *payloadUD = createTempVar(numElts, Type_UD, getGRFAlign());
    payloadUD->setAliasDeclare(payloadF, 0);

    // even though we only use lower half of the GRF, we have to allocate full
    // GRF
    G4_Declare *payloadHF = createTempVar(numElts * 2, Type_HF, Any);
    payloadHF->setAliasDeclare(payloadF, 0);

    G4_SrcRegRegion *srcToUse2 =
        createSrc(payloadUD->getRegVar(), 0, 0, getRegionStride1(), Type_UD);

    if (useHeader) {
      createMovInst(payloadUD, 0, 0, g4::SIMD8, nullptr, nullptr,
                    createSrcRegRegion(header, getRegionStride1()), true);
    }

    for (unsigned int i = 0; i < numParms; i++) {
      if (params[i]->isNullReg()) {
        secondHalf[i] = params[i];
      } else if (params[i]->getTypeSize() == 2) {
        // V1(0,8)<8;8,1>
        secondHalf[i] = createSrcWithNewSubRegOff(params[i], execSize);
      } else {
        // V1(1,0)<8;8,1>
        secondHalf[i] =
            createSrcWithNewRegOff(params[i], params[i]->getRegOff() + 1);
      }
    }

    regOff = (useHeader ? 1 : 0);
    for (unsigned paramCounter = 0; paramCounter < numParms; ++paramCounter) {
      temp = secondHalf[paramCounter];
      uint32_t MovInstOpt = InstOpt_WriteEnable;

      if (temp->getTypeSize() == 2) {
        // we should generate
        // mov (8) dst<1>:hf src.8<8;8,1>:hf
        G4_DstRegRegion *dstHF =
            createDst(payloadHF->getRegVar(), regOff++, 0, 1, temp->getType());
        createMov(execSize, dstHF, temp, MovInstOpt, true);
      } else {
        Copy_SrcRegRegion_To_Payload(payloadF, regOff, temp, execSize,
                                     MovInstOpt);
      }
    }

    G4_Operand *surface2 = duplicateOperand(surface);

    // sampler may be null for 3d load (specifically ld2dms_w)
    G4_Operand *sampler2 =
        sampler == nullptr ? nullptr : duplicateOperand(sampler);

    G4_Predicate *pred2 = dupPredicate(pred);

    G4_SendDescRaw *msgDesc2 =
        createSampleMsgDesc(desc, cpsEnable, 0, surface2, sampler2);
    msgDesc2->setHeaderPresent(useHeader);

    if (forceSplitSend) {
      sendInst =
          createSplitSendInst(pred2, dst2, srcToUse2, createNullSrc(Type_UD),
                              execSize, msgDesc2, instOpt2, false);
    } else {
      sendInst = createSendInst(pred2, dst2, srcToUse2, execSize, msgDesc2,
                                instOpt2, false);
    }
    setUniformSampler(sendInst, uniformSampler);
  }

  {

    /**************** MOVING FROM TEMP TO DST, 1st half *********************/
    regOff = 0;
    for (unsigned i = 0; i < tmpDstRows; i++, regOff += 1) {
      // If Pixel Null Mask is enabled, then only copy the last double word.
      if (pixelNullMaskEnable && i == tmpDstRows - 1) {
        G4_DstRegRegion *origDstPtr =
            createDst(origDstUD->getRegVar(), short(regOff), 0, 1, Type_UD);
        G4_SrcRegRegion *src0Ptr = createSrc(tempDstUD->getRegVar(), short(i),
                                             0, getRegionScalar(), Type_UD);

        G4_Predicate *pred2 = dupPredicate(pred);

        // Copy the write mask message W4.0 into the dst. (No mask?)
        createInst(pred2, G4_mov, NULL, g4::NOSAT, g4::SIMD1, origDstPtr,
                   src0Ptr, NULL, NULL, InstOpt_WriteEnable, true);
        // Skip the remaining part of the loop.
        break;
      }

      G4_SrcRegRegion *tmpSrcPnt =
          createSrc(tempDstDcl->getRegVar(), (short)i, 0, getRegionStride1(),
                    tempDstDcl->getElemType());

      uint32_t MovInstOpt = instOpt;
      if (isHalfReturn) {
        // mov (8) dst(0,0)<1>:hf tmp(0,0)<8;8,1>:hf {Q1}
        G4_DstRegRegion *dst =
            createDst(originalDstDcl->getRegVar(), (short)regOff, 0, 1,
                      originalDstDcl->getElemType());
        createMov(execSize, dst, tmpSrcPnt, MovInstOpt, true);
      } else {
        Copy_SrcRegRegion_To_Payload(originalDstDcl, regOff, tmpSrcPnt,
                                     execSize, MovInstOpt, pred);
      }
    }
  }

  {
    /**************** MOVING FROM TEMP TO DST, 2nd half *********************/
    regOff = isHalfReturn ? 0 : 1;
    for (unsigned i = 0; i < tmpDstRows; i++, regOff += 1) {
      // If Pixel Null Mask is enabled, copy the second half to the originai dst
      if (pixelNullMaskEnable && i == tmpDstRows - 1) {
        G4_Type secondHalfType = execSize == g4::SIMD8 ? Type_UB : Type_UW;
        G4_DstRegRegion *origDstPtr =
            createDst(origDstUD->getRegVar(), regOff - 1, 1, 1, secondHalfType);
        G4_SrcRegRegion *src0Ptr =
            createSrc(tempDst2UD->getRegVar(), short(i), 0, getRegionScalar(),
                      secondHalfType);

        G4_Predicate *pred2 = dupPredicate(pred);
        // write to dst.0[8:15]
        createInst(pred2, G4_mov, NULL, g4::NOSAT, g4::SIMD1, origDstPtr,
                   src0Ptr, NULL, InstOpt_WriteEnable, true);

        // Skip the remaining part of the loop.
        break;
      }

      G4_SrcRegRegion *tmpSrcPnt =
          createSrc(tempDstDcl2->getRegVar(), (short)i, 0, getRegionStride1(),
                    tempDstDcl->getElemType());

      uint32_t MovInstOpt = instOpt2;
      if (isHalfReturn) {
        // mov (8) dst(0,8)<1>:hf tmp(0,0)<8;8,1>:hf {Q2}
        G4_DstRegRegion *dst =
            createDst(originalDstDcl->getRegVar(), (short)regOff, execSize, 1,
                      originalDstDcl->getElemType());
        createMov(execSize, dst, tmpSrcPnt, MovInstOpt, true);
      } else {
        Copy_SrcRegRegion_To_Payload(originalDstDcl, regOff, tmpSrcPnt,
                                     execSize, MovInstOpt, pred);
      }
    }
  }
  return status;
}

void IR_Builder::doSamplerHeaderMove(G4_Declare *headerDcl,
                                     G4_Operand *sampler) {
  if (isBindlessSampler(sampler)) {
    // sampler index in msg desc will be 0, manipulate the sampler offset
    // instead mov (1) M0.3<1>:ud sampler<0;1,0>:ud the driver will send the
    // handle with bit 0 already set
    G4_DstRegRegion *dst = createDst(headerDcl->getRegVar(), 0, 3, 1, Type_UD);
    createMov(g4::SIMD1, dst, sampler, InstOpt_WriteEnable, true);
  }
}


void vISA::IR_Builder::doPairedResourceHeaderMove(G4_Declare *headerDcl,
                                                  G4_Operand *pairedResource) {
  if (hasSamplerFeedbackSurface()) {
    if (pairedResource->isNullReg() == false) {
      G4_DstRegRegion *dstDword4 =
          createDst(headerDcl->getRegVar(), 0, 4, 1, Type_UD);
      // mov (1) M0.4<1>:ud pairedResource<0;1,0>:ud
      createMov(g4::SIMD1, dstDword4, pairedResource, InstOpt_WriteEnable,
                true);
      // or (1) M0.4<1>:ud M0.4<1>:ud 0x1
      dstDword4 = createDst(headerDcl->getRegVar(), 0, 4, 1, Type_UD);
      G4_SrcRegRegion *srcDword4 =
          createSrc(headerDcl->getRegVar(), 0, 4, getRegionScalar(), Type_UD);
      createBinOp(G4_or, g4::SIMD1, dstDword4, srcDword4,
                  createImm(0x1, Type_UD), InstOpt_WriteEnable, true);
    } else {
      // Clear sampler feedback surface data - dword #4 in header.
      G4_DstRegRegion *dst =
          createDst(headerDcl->getRegVar(), 0, 4, 1, Type_UD);
      createMov(g4::SIMD1, dst, createImm(0, Type_UD), InstOpt_WriteEnable,
                true);
    }
  }
}

//
// generate the r0 move for the sampler message header, and return the dcl
// for CNL+, also set SSP to dynamic if message is not bindless
//
G4_Declare *IR_Builder::getSamplerHeader(bool isBindlessSampler,
                                         bool samplerIndexGE16) {
  G4_Declare *dcl = nullptr;

  G4_InstOpts dbgOpt = m_options->getOption(vISA_markSamplerMoves)
                           ? InstOpt_BreakPoint
                           : InstOpt_NoOpt;
  if (m_options->getOption(vISA_cacheSamplerHeader) && !isBindlessSampler) {
    dcl = builtinSamplerHeader;
    if (!builtinSamplerHeaderInitialized) {
      builtinSamplerHeaderInitialized = true;
      if (hasBindlessSampler()) {
        // make sure we set bit 0 of M0.3:ud to be 0
        // and (1) M0.6<1>:uw M0.6<1>:uw 0xFFFE
        G4_DstRegRegion *dst = createDst(dcl->getRegVar(), 0, 6, 1, Type_UW);
        G4_SrcRegRegion *src0 =
            createSrc(dcl->getRegVar(), 0, 6, getRegionScalar(), Type_UW);
        G4_INST *SSPMove =
            createBinOp(G4_and, g4::SIMD1, dst, src0,
                        createImm(0xFFFE, Type_UW), InstOpt_WriteEnable, false);
        instList.push_front(SSPMove);
      }
      G4_INST *r0Move =
          createMov(g4::SIMD8, createDstRegRegion(dcl, 1),
                    createSrcRegRegion(builtinR0, getRegionStride1()),
                    InstOpt_WriteEnable | dbgOpt, false);
      instList.push_front(r0Move);
    }
    if (samplerIndexGE16) {
      // When sampler index is greater or equal 16 then the
      // createSamplerHeader() message overwrites the sampler states
      // pointer in the header -> cannot use the cached value in this
      // case.
      dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);
      dcl->setCapableOfReuse();
      G4_SrcRegRegion *src = createSrc(builtinSamplerHeader->getRegVar(), 0, 0,
                                       getRegionStride1(), Type_UD);
      createMovInst(dcl, 0, 0, g4::SIMD8, NULL, NULL, src, false, dbgOpt);
    }
  } else {
    dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);
    dcl->setCapableOfReuse();
    createMovR0Inst(dcl, 0, 0, true, dbgOpt);
    if (hasBindlessSampler() && !isBindlessSampler) {
      // make sure we set bit 0 of M0.3:ud to be 0
      // and (1) M0.6<1>:uw M0.6<1>:uw 0xFFFE
      G4_DstRegRegion *dst = createDst(dcl->getRegVar(), 0, 6, 1, Type_UW);
      G4_SrcRegRegion *src0 =
          createSrc(dcl->getRegVar(), 0, 6, getRegionScalar(), Type_UW);
      createBinOp(G4_and, g4::SIMD1, dst, src0, createImm(0xFFFE, Type_UW),
                  InstOpt_WriteEnable, true);
    }
  }

  return dcl;
}

// get the number of GRFs occupied by a sampler message's operand
static uint32_t getNumGRF(unsigned grfSize, bool isFP16, int execSize) {
  int numBytes = (isFP16 ? 2 : 4) * execSize;
  return (numBytes + grfSize - 1) / grfSize;
}

uint32_t IR_Builder::getSamplerResponseLength(int numChannels, bool isFP16,
                                              int execSize, bool pixelNullMask,
                                              bool nullDst) {
  if (nullDst) {
    hasNullReturnSampler = true;
    return 0;
  }
  uint32_t responseLength =
      numChannels * getNumGRF(getGRFSize(), isFP16, execSize);

  if (pixelNullMask) {
    ++responseLength;
  }
  return responseLength;
}

static bool needSamplerHeader(IR_Builder *builder, bool pixelNullMask,
                              bool nonZeroAoffImmi, bool needHeaderForChannels,
                              bool bindlessSampler, bool hasPairedSurface,
                              bool simd16HFReturn) {
  return builder->forceSamplerHeader() ||
         (pixelNullMask && builder->hasPixelNullMask()) || nonZeroAoffImmi ||
         needHeaderForChannels || bindlessSampler || hasPairedSurface ||
         (simd16HFReturn && VISA_WA_CHECK(builder->getPWaTable(),
                                          WaHeaderRequiredOnSimd16Sample16bit));
}

// This function assumes there are no gaps in parameter array. e.g. NULL
// pointers If there is a gap it must be RawOperand with value 0.
int IR_Builder::translateVISASampler3DInst(
    VISASampler3DSubOpCode actualop, bool pixelNullMask, bool cpsEnable,
    bool uniformSampler, G4_Predicate *pred, VISA_Exec_Size executionSize,
    VISA_EMask_Ctrl emask, ChannelMask chMask, G4_Operand *aoffimmi,
    G4_Operand *sampler, G4_Operand *surface, G4_Operand *pairedSurface,
    G4_DstRegRegion *dst, unsigned int numParms, G4_SrcRegRegion **params) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize execSize = toExecSize(executionSize);
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, execSize);

  // First setup message header and message payload

  // Message header and payload size is numParms GRFs

  const bool FP16Return = dst->getTypeSize() == 2;
  const bool FP16Input = params[0]->getType() == Type_HF;

  bool useHeader = false;

  unsigned int numRows =
      numParms * getNumGRF(getGRFSize(), FP16Input, execSize);

  VISAChannelMask channels = chMask.getAPI();
  // For SKL+ channel mask R, RG, RGB, and RGBA may be derived from response
  // length
  bool needHeaderForChannels =
      (getPlatform() < GENX_SKL)
          ? channels != CHANNEL_MASK_RGBA
          : (channels != CHANNEL_MASK_R && channels != CHANNEL_MASK_RG &&
             channels != CHANNEL_MASK_RGB && channels != CHANNEL_MASK_RGBA);

  bool nonZeroAoffImmi =
      !(aoffimmi->isImm() && aoffimmi->asImm()->getInt() == 0);
  bool simd16HFReturn = FP16Return && execSize == 16;
  if (useHeader ||
      needSamplerHeader(this, pixelNullMask, nonZeroAoffImmi,
                        needHeaderForChannels, isBindlessSampler(sampler),
                        !pairedSurface->isNullReg(), simd16HFReturn) ||
      samplerHeaderPreemptionWA()) {
    useHeader = true;
    ++numRows;
  }

  int numChannels = chMask.getNumEnabledChannels();

  if (execSize > getNativeExecSize() &&
      (numRows > 11 || actualop == VISA_3D_SAMPLE_D ||
       actualop == VISA_3D_SAMPLE_D_C || actualop == VISA_3D_SAMPLE_KILLPIX)) {
    // decrementing since we will produce SIMD8 code.
    // don't do this for SIMD16H since its message length is the same as SIMD8H
    if (!FP16Input) {
      numRows -= numParms;
    }

    return splitSampleInst(actualop, pixelNullMask, cpsEnable, pred, chMask,
                           numChannels, aoffimmi, sampler, surface,
                           pairedSurface, dst, emask, useHeader, numRows,
                           numParms, params, uniformSampler);
  }

  bool useSplitSend = useSends();

  G4_SrcRegRegion *header = 0;
  G4_Operand *samplerIdx = sampler;

    if (useHeader) {
      const bool samplerIndexGE16 = IsSamplerIndexGE16(aoffimmi);
      G4_Declare *dcl =
          getSamplerHeader(isBindlessSampler(sampler), samplerIndexGE16);
      samplerIdx = createSampleHeader(this, dcl, actualop, pixelNullMask,
                                      aoffimmi, chMask,
                                      pairedSurface,
                                      sampler);
      header = createSrcRegRegion(dcl, getRegionStride1());
    }

  G4_InstOpts dbgOpt = m_options->getOption(vISA_markSamplerMoves)
                           ? InstOpt_BreakPoint
                           : InstOpt_NoOpt;
  // Collect payload sources.
  unsigned len = numParms + (header ? 1 : 0);
  std::vector<PayloadSource> sources(len);
  unsigned i = 0;
  // Collect header if present.
  if (header) {
    sources[i].opnd = header;
    sources[i].numElts = g4::SIMD8;
    sources[i].instOpt = InstOpt_WriteEnable | dbgOpt;
    ++i;
  }
  // Collect all parameters.
  bool needNoMask = needsNoMaskCoordinates(actualop);
  unsigned uPos = needNoMask ? getUPosition(actualop) : ~0u;
  for (unsigned j = 0; j != numParms; ++j) {
    sources[i].opnd = params[j];
    sources[i].numElts = execSize;
    sources[i].instOpt = (needNoMask && (uPos <= j && j < (uPos + 3)))
                             ? InstOpt_WriteEnable | dbgOpt
                             : instOpt | dbgOpt;
    ++i;
  }
  vISA_ASSERT_INPUT(i == len,
                    "There's mismatching during payload source collecting!");

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, execSize, useSplitSend, sources.data(), len);

  uint32_t responseLength = getSamplerResponseLength(
      numChannels, FP16Return, execSize, hasPixelNullMask() && pixelNullMask,
      dst->isNullReg());

  // Check if CPS LOD Compensation Enable is valid.
  if (cpsEnable) {
    checkCPSEnable(actualop, responseLength, execSize);
  }

  uint32_t fc = createSamplerMsgDesc(actualop, execSize == getNativeExecSize(),
                                     FP16Return, FP16Input);
  uint32_t desc =
      G4_SendDescRaw::createDesc(fc, useHeader, sizes[0], responseLength);

  G4_InstSend *sendInst = nullptr;
  bool forceSplitSend = shouldForceSplitSend(surface);
  if (msgs[1] == 0 && !forceSplitSend) {
    vISA_ASSERT_INPUT(sizes[1] == 0,
                      "Expect the 2nd part of the payload has zero size!");
    G4_SendDescRaw *msgDesc =
        createSampleMsgDesc(desc, cpsEnable, 0, surface, samplerIdx);

    sendInst =
        createSendInst(pred, dst, msgs[0], execSize, msgDesc, instOpt, false);
  } else {
    G4_SendDescRaw *msgDesc =
        createSampleMsgDesc(desc, cpsEnable, sizes[1], surface, samplerIdx);
    sendInst = createSplitSendInst(pred, dst, msgs[0], msgs[1], execSize,
                                   msgDesc, instOpt, false);
  }
  setUniformSampler(sendInst, uniformSampler);
  return VISA_SUCCESS;
}

int IR_Builder::translateVISALoad3DInst(
    VISASampler3DSubOpCode actualop, bool pixelNullMask,
    G4_Predicate *pred_opnd, VISA_Exec_Size executionSize, VISA_EMask_Ctrl em,
    ChannelMask channelMask, G4_Operand *aoffimmi, G4_Operand *surface,
    G4_Operand *pairedSurface, G4_DstRegRegion *dst, uint8_t numParms,
    G4_SrcRegRegion **opndArray) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  bool useHeader = false;

  G4_ExecSize execSize = toExecSize(executionSize);
  G4_InstOpts instOpt = Get_Gen4_Emask(em, execSize);

  const bool halfReturn = dst->getTypeSize() == 2;
  const bool halfInput = opndArray[0]->getTypeSize() == 2;

  unsigned int numRows =
      numParms * getNumGRF(getGRFSize(), halfInput, execSize);

  VISAChannelMask channels = channelMask.getAPI();
  // For SKL+ channel mask R, RG, RGB, and RGBA may be derived from response
  // length
  bool needHeaderForChannels =
      (getPlatform() < GENX_SKL)
          ? channels != CHANNEL_MASK_RGBA
          : (channels != CHANNEL_MASK_R && channels != CHANNEL_MASK_RG &&
             channels != CHANNEL_MASK_RGB && channels != CHANNEL_MASK_RGBA);

  bool nonZeroAoffImmi =
      !(aoffimmi->isImm() && aoffimmi->asImm()->getInt() == 0);
  bool simd16HFReturn = halfReturn && execSize == 16;
  if (useHeader ||
      needSamplerHeader(this, pixelNullMask, nonZeroAoffImmi,
                        needHeaderForChannels, false,
                        !pairedSurface->isNullReg(), simd16HFReturn)) {
    useHeader = true;
    ++numRows;
  }

  int numChannels = channelMask.getNumEnabledChannels();
  if (execSize > getNativeExecSize() && numRows > 11) {
    // decrementing since we will produce SIMD8 code.
    // don't do this for SIMD16H since its message length is the same as SIMD8H
    if (!halfInput) {
      numRows -= numParms;
    }
    return splitSampleInst(actualop, pixelNullMask, /*cpsEnable*/ false,
                           pred_opnd, channelMask, numChannels, aoffimmi, NULL,
                           surface, pairedSurface, dst, em, useHeader, numRows,
                           numParms, opndArray);
  }

  bool useSplitSend = useSends();

  G4_SrcRegRegion *header = nullptr;
  if (useHeader) {
    G4_Declare *dcl = getSamplerHeader(false /*isBindlessSampler*/,
                                       false /*samperIndexGE16*/);
    {
      (void)createSampleHeader(this, dcl, actualop, pixelNullMask, aoffimmi,
                               channelMask, pairedSurface, nullptr);
    }
    header = createSrcRegRegion(dcl, getRegionStride1());
  }

  // Collect payload sources.
  unsigned len = numParms + (header ? 1 : 0);
  std::vector<PayloadSource> sources(len);
  unsigned i = 0;
  // Collect header if present.
  if (header) {
    sources[i].opnd = header;
    sources[i].numElts = g4::SIMD8;
    sources[i].instOpt = InstOpt_WriteEnable;
    ++i;
  }
  // Collect all parameters.
  bool needNoMask = needsNoMaskCoordinates(actualop);
  unsigned uPos = needNoMask ? getUPosition(actualop) : ~0u;
  for (unsigned j = 0; j != numParms; ++j) {
    sources[i].opnd = opndArray[j];
    sources[i].numElts = execSize;
    sources[i].instOpt = (needNoMask && (uPos <= j && j < (uPos + 3)))
                             ? InstOpt_WriteEnable
                             : instOpt;
    ++i;
  }
  vISA_ASSERT_INPUT(i == len,
                    "There's mismatching during payload source collecting!");

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, execSize, useSplitSend, sources.data(), len);

  uint32_t fc = createSamplerMsgDesc(actualop, execSize == getNativeExecSize(),
                                     halfReturn, halfInput);

  uint32_t responseLength = getSamplerResponseLength(
      numChannels, halfReturn, execSize, hasPixelNullMask() && pixelNullMask,
      dst->isNullReg());

  bool forceSplitSend = shouldForceSplitSend(surface);
  if (msgs[1] == 0 && !forceSplitSend) {
    createSendInst(pred_opnd, dst, msgs[0], sizes[0], responseLength, execSize,
                   fc, SFID::SAMPLER, useHeader, SendAccess::READ_ONLY, surface,
                   NULL, instOpt, false);
  } else {
    createSplitSendInst(pred_opnd, dst, msgs[0], sizes[0], msgs[1], sizes[1],
                        responseLength, execSize, fc, SFID::SAMPLER, useHeader,
                        SendAccess::READ_ONLY, surface, NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISAGather3dInst(
    VISASampler3DSubOpCode actualop, bool pixelNullMask, G4_Predicate *pred,
    VISA_Exec_Size executionSize, VISA_EMask_Ctrl em, ChannelMask channelMask,
    G4_Operand *aoffimmi, G4_Operand *sampler, G4_Operand *surface,
    G4_Operand *pairedSurface, G4_DstRegRegion *dst, unsigned int numOpnds,
    G4_SrcRegRegion **opndArray) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  bool useHeader = false;

  G4_ExecSize execSize = toExecSize(executionSize);
  G4_InstOpts instOpt = Get_Gen4_Emask(em, execSize);

  const bool FP16Return = dst->getTypeSize() == 2;
  const bool FP16Input = opndArray[0]->getType() == Type_HF;

  unsigned int numRows =
      numOpnds * getNumGRF(getGRFSize(), FP16Input, execSize);

  bool nonZeroAoffImmi =
      !(aoffimmi->isImm() && aoffimmi->asImm()->getInt() == 0);
  bool needHeaderForChannels =
      channelMask.getSingleChannel() != VISA_3D_GATHER4_CHANNEL_R;
  bool simd16HFReturn = FP16Return && execSize == 16;

  if (useHeader ||
      needSamplerHeader(this, pixelNullMask, nonZeroAoffImmi,
                        needHeaderForChannels, isBindlessSampler(sampler),
                        !pairedSurface->isNullReg(), simd16HFReturn) ||
      samplerHeaderPreemptionWA()) {
    useHeader = true;
    ++numRows;
  }


  if (execSize > getNativeExecSize() && numRows > 11) {
    // decrementing since we will produce SIMD8 code.
    // don't do this for SIMD16H since its message length is the same as SIMD8H
    if (!FP16Input) {
      numRows -= numOpnds;
    }

    return splitSampleInst(actualop, pixelNullMask, /*cpsEnable*/ false, pred,
                           channelMask, 4, aoffimmi, sampler, surface,
                           pairedSurface,
                           dst, em, useHeader, numRows, numOpnds, opndArray);
  }

  bool useSplitSend = useSends();

  G4_SrcRegRegion *header = nullptr;
  G4_Operand *samplerIdx = sampler;

  if (useHeader) {
    const bool samplerIndexGE16 = IsSamplerIndexGE16(aoffimmi);
    G4_Declare *dcl =
        getSamplerHeader(isBindlessSampler(sampler), samplerIndexGE16);
    {
      samplerIdx = createSampleHeader(this, dcl, actualop, pixelNullMask,
                                      aoffimmi, channelMask,
                                      pairedSurface,
                                      sampler);
    }
    header = createSrcRegRegion(dcl, getRegionStride1());
  }

  // Collect payload sources.
  unsigned len = numOpnds + (header ? 1 : 0);
  std::vector<PayloadSource> sources(len);
  unsigned i = 0;
  // Collect header if present.
  if (header) {
    sources[i].opnd = header;
    sources[i].numElts = g4::SIMD8;
    sources[i].instOpt = InstOpt_WriteEnable;
    ++i;
  }
  // Collect all parameters.
  bool needNoMask = needsNoMaskCoordinates(actualop);
  unsigned uPos = needNoMask ? getUPosition(actualop) : ~0u;
  for (unsigned j = 0; j != numOpnds; ++j) {
    sources[i].opnd = opndArray[j];
    sources[i].numElts = execSize;
    sources[i].instOpt = (needNoMask && (uPos <= j && j < (uPos + 3)))
                             ? InstOpt_WriteEnable
                             : instOpt;
    ++i;
  }
  vISA_ASSERT_INPUT(i == len,
                    "There's mismatching during payload source collecting!");

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, execSize, useSplitSend, sources.data(), len);

  uint32_t fc = createSamplerMsgDesc(actualop, execSize == getNativeExecSize(),
                                     FP16Return, FP16Input);
  uint32_t responseLength = getSamplerResponseLength(
      4, FP16Return, execSize, hasPixelNullMask() && pixelNullMask,
      dst->isNullReg());

  bool forceSplitSend = shouldForceSplitSend(surface);
  if (msgs[1] == 0 && !forceSplitSend) {
    createSendInst(pred, dst, msgs[0], sizes[0], responseLength, execSize, fc,
                   SFID::SAMPLER, useHeader, SendAccess::READ_ONLY, surface,
                   samplerIdx, instOpt, false);
  } else {
    createSplitSendInst(pred, dst, msgs[0], sizes[0], msgs[1], sizes[1],
                        responseLength, execSize, fc, SFID::SAMPLER, useHeader,
                        SendAccess::READ_ONLY, surface, samplerIdx, instOpt,
                        false);
  }

  return VISA_SUCCESS;
}

/*
 * Translates Sampler Norm API intrinsic.
 *
 * Assuming: N = 4, channelMask=ABGR_ENABLE, surfIndex = 0x21, samplerIndex =
 * 0x4, then the generated code should look like the following for GT:
 *
 * .declare  VX Base=m ElementSize=4 Type=ud Total=16
 * .declare  VY Base=r ElementSize=2 Type=uw Total=128
 *
 * mov  (8)     VX(0,0)<1>,  r0:ud
 * mov  (1)     VX(0,2)<1>,  0
 * mov  (1)     VX(1,1)<1>,  deltaU
 * mov  (1)     VX(1,2)<1>,  u
 * mov  (1)     VX(1,5)<1>,  deltaV
 * mov  (1)     VX(1,6)<1>,  v
 * send (16)    VY(0,0)<1>,  VX(0,0),    0x2,   0x048bc421
 * mov  (128)   M(0,0)<1>,   VY(0,0)
 *
 * VX(0,0): message header
 *
 * VX(1,0): SIMD32 media payload
 *
 * ex_desc: 0x2 == 0010 (Target Function ID: Sampling Engine)
 *
 * desc: 0x048bc421 == Bit 31-29: 000 (Reserved)
 *                     Bit 28-25: 0010 (Message Length =)
 *                     Bit 24-20: 01000 (Response Message Length = 8)
 *                     Bit 19:    1 (Header present)
 *                     Bit 18:    0 (Reserved)
 *                     Bit 17-16: 11 (SIMD Mode = SIMD32)
 *                     Bit 15-12: 1100 (Message Type = sample_unorm media)
 *                     Bit 11-8:  0000 + samplerIndex  (Sampler Index)
 *                     Bit 7-0:   00000000 + surfIndex (Binding Table Index)
 *
 */
int IR_Builder::translateVISASamplerNormInst(
    G4_Operand *surface, G4_Operand *sampler, ChannelMask channel,
    unsigned numEnabledChannels, G4_Operand *deltaUOpnd, G4_Operand *uOffOpnd,
    G4_Operand *deltaVOpnd, G4_Operand *vOffOpnd, G4_DstRegRegion *dst_opnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  // mov (8)      VX(0,0)<1>,  r0:ud
  // add dcl for VX
  G4_Declare *dcl = createSendPayloadDcl(2 * getGenxSamplerIOSize(), Type_UD);

  // mov  VX(0,0)<1>, r0
  createMovR0Inst(dcl, 0, 0);
  /* mov (1)     VX(0,2)<1>,   0  */
  unsigned cmask = channel.getHWEncoding() << 12;
  createMovInst(dcl, 0, 2, g4::SIMD1, NULL, NULL, createImm(cmask, Type_UD));

  G4_Declare *dcl1 = createSendPayloadDcl(getGenxDataportIOSize(), Type_F);
  dcl1->setAliasDeclare(dcl, numEltPerGRF<Type_UB>());

  // mov  (1)     VX(1,4)<1>,  deltaU
  createMovInst(dcl1, 0, 4, g4::SIMD1, NULL, NULL, deltaUOpnd);
  // mov  (1)     VX(1,2)<1>,  u
  createMovInst(dcl1, 0, 2, g4::SIMD1, NULL, NULL, uOffOpnd);
  // mov  (1)     VX(1,5)<1>,  deltaV
  createMovInst(dcl1, 0, 5, g4::SIMD1, NULL, NULL, deltaVOpnd);
  // mov  (1)     VX(1,3)<1>,  v
  createMovInst(dcl1, 0, 3, g4::SIMD1, NULL, NULL, vOffOpnd);

  // send's operands preparation
  // create a currDst for VX
  G4_SrcRegRegion *payload = createSrcRegRegion(dcl, getRegionStride1());

  G4_DstRegRegion *d = checkSendDst(dst_opnd->asDstRegRegion());

  // Set bit 12-17 for the message descriptor
  unsigned descFc = 0;
  descFc |= 0xC << 12; // Bit 16-12 = 1100 for Sampler Message Type
  descFc |= 0x3 << 17; // Bit 18-17 = 11 for SIMD32 mode

  createSendInst(NULL, d, payload, 2,
                 32 * numEnabledChannels * TypeSize(Type_UW) /
                     numEltPerGRF<Type_UB>(),
                 g4::SIMD32, descFc, SFID::SAMPLER, 1, SendAccess::READ_ONLY,
                 surface, sampler, 0, false);

  return VISA_SUCCESS;
}

/*
 * Translates Sampler intrinsic.
 *
 * Assuming: N = 4, channelMask=ABGR_ENABLE, surfIndex = 0x21, samplerIndex =
 * 0x4, then the generated code should look like the following for GT:
 *
 * .declare  VX Base=m ElementSize=4 Type=f Total=72
 * .declare  VY Base=r ElementSize=4 Type=f Total=64
 * .declare  VZ Base=r ElementSize=2 Type=w Total=128 ALIAS(VY,0)
 *
 * mov  (8)     VX(0,0)<1>,  r0:ud
 * mov  (1)     VX(0,2)<1>,  0
 * mov  (16)    VX(1,0)<1>,  u
 * mov  (16)    VX(3,0)<1>,  v
 * mov  (16)    VX(5,0)<1>,  r
 * mov  (16)    VX(7,0)<1>,  0
 * send (16)    VY(0,0)<1>,  VX(0,0),    0x2,  0x128a0421
 * mov  (64)    M(0,0)<1>,   VY(0,0)
 *
 * ex_desc: 0x2 == 0010 (Target Function ID: Sampling Engine)
 *
 * desc: 0x128a0421 == Bit 31-29: 000 (Reserved)
 *                     Bit 28-25: 1001 (Message Length = 9 (1+2*4 for SIMD16))
 *                     Bit 24-20: 01000 (Response Message Length = 8)
 *                     Bit 19:    1 (Header present)
 *                     Bit 18:    0 (Reserved)
 *                     Bit 17-16: 10 (SIMD Mode = SIMD16)
 *                     Bit 15-12: 0000 (Message Type = Sample)
 *                     Bit 11-8:  0000 + samplerIndex  (Sampler Index)
 *                     Bit 7-0:   00000000 + surfIndex (Binding Table Index)
 *
 */
int IR_Builder::translateVISASamplerInst(
    unsigned simdMode, G4_Operand *surface, G4_Operand *sampler,
    ChannelMask channel, unsigned numEnabledChannels, G4_Operand *uOffOpnd,
    G4_Operand *vOffOpnd, G4_Operand *rOffOpnd, G4_DstRegRegion *dstOpnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  // mov (8)      VX(0,0)<1>,  r0:ud
  // add dcl for VX
  unsigned num_payload_elt =
      simdMode / 2 * numEltPerGRF<Type_UB>() / TypeSize(Type_UD);
  G4_Declare *dcl =
      createSendPayloadDcl(num_payload_elt + getGenxSamplerIOSize(), Type_UD);

  // mov  VX(0,0)<1>, r0
  createMovR0Inst(dcl, 0, 0);
  unsigned cmask = channel.getHWEncoding() << 12;
  /* mov (1)     VX(0,2)<1>,   0  */
  createMovInst(dcl, 0, 2, g4::SIMD1, NULL, NULL, createImm(cmask, Type_UD));

  // set up the message payload
  // lod is always uninitialized for us as we don't support it.
  G4_Declare *dcl1 = createSendPayloadDcl(num_payload_elt, Type_UD);
  dcl1->setAliasDeclare(dcl, numEltPerGRF<Type_UB>());
  /* mov  (sample_mode)    VX(0,0)<1>,  u */
  createMovSendSrcInst(dcl1, 0, 0, simdMode, uOffOpnd, 0);
  if (sampler == NULL) {
    // ld
    if (getPlatform() < GENX_SKL) {
      // the order of paramters is
      // u    lod        v    r
      /* mov  (sample_mode)    VX(sample_mode/8, 0)<1>,  lod */
      createMovSendSrcInst(dcl1, simdMode / 8, 0, simdMode,
                           createImm(0, Type_UD), 0);
      /* mov  (sample_mode)    VX(2*sample_mode/8, 0)<1>,  v */
      createMovSendSrcInst(dcl1, 2 * simdMode / 8, 0, simdMode, vOffOpnd, 0);
      /* mov  (sample_mode)    VX(3*sampler_mode/8, 0)<1>,  r */
      createMovSendSrcInst(dcl1, 3 * simdMode / 8, 0, simdMode, rOffOpnd, 0);
    } else {
      // SKL+: the order of paramters is
      // u    v   lod r
      /* mov  (sample_mode)    VX(sample_mode/8, 0)<1>,  v */
      createMovSendSrcInst(dcl1, simdMode / 8, 0, simdMode, vOffOpnd, 0);
      /* mov  (sample_mode)    VX(2*sample_mode/8, 0)<1>,  lod */
      createMovSendSrcInst(dcl1, 2 * simdMode / 8, 0, simdMode,
                           createImm(0, Type_UD), 0);
      /* mov  (sample_mode)    VX(3*sampler_mode/8, 0)<1>,  r */
      createMovSendSrcInst(dcl1, 3 * simdMode / 8, 0, simdMode, rOffOpnd, 0);
    }
  } else {
    // sample
    /* mov  (sample_mode)    VX(1 + sample_mode/8, 0)<1>,  v */
    createMovSendSrcInst(dcl1, simdMode / 8, 0, simdMode, vOffOpnd, 0);
    /* mov  (sample_mode)    VX(3,0)<1>,  r */
    createMovSendSrcInst(dcl1, 2 * simdMode / 8, 0, simdMode, rOffOpnd, 0);
    /* mov  (sample_mode)    VX(5,0)<1>,  0 */
    createMovSendSrcInst(dcl1, 3 * simdMode / 8, 0, simdMode,
                         createImm(0, Type_UD), 0);
  }
  // send's operands preparation
  // create a currDst for VX
  G4_SrcRegRegion *payload = createSrcRegRegion(dcl, getRegionStride1());

  G4_DstRegRegion *d = checkSendDst(dstOpnd->asDstRegRegion());

  // Set bit 9-8 for the message descriptor
  unsigned descFc = 0;

  // Bit 17-18 = 10 for SIMD mode
  if (simdMode == 8) {
    descFc |= 0x1 << 17;
  } else {
    descFc |= 0x2 << 17;
  }

  if (sampler == nullptr) {
    static const unsigned SAMPLER_MESSAGE_TYPE_OFFSET = 12;
    // LD message
    descFc += VISASampler3DSubOpCode::VISA_3D_LD << SAMPLER_MESSAGE_TYPE_OFFSET;
  }

  if (simdMode == 16) {
    // redefine the type and offset of post dst.
    if ((d->getType() != Type_W) && (d->getType() != Type_UW)) {
      short new_SubRegOff = dstOpnd->asDstRegRegion()->getSubRegOff();
      if (dstOpnd->getRegAccess() == Direct) {
        new_SubRegOff = (dstOpnd->asDstRegRegion()->getSubRegOff() *
                         dstOpnd->getTypeSize()) /
                        TypeSize(Type_W);
      }
      G4_DstRegRegion new_dst(
          *this, dstOpnd->getRegAccess(), dstOpnd->asDstRegRegion()->getBase(),
          dstOpnd->asDstRegRegion()->getRegOff(), new_SubRegOff, 1, Type_W);
      d = createDstRegRegion(new_dst);
    }
  }

  createSendInst(NULL, d, payload, 1 + simdMode / 2,
                 ((simdMode == 8) ? 32 : (numEnabledChannels * 16)) *
                     TypeSize(Type_F) / numEltPerGRF<Type_UB>(),
                 G4_ExecSize(simdMode), descFc, SFID::SAMPLER, 1,
                 SendAccess::READ_ONLY, surface, sampler, 0, false);
  return VISA_SUCCESS;
}
