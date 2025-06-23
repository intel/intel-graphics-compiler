/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../Timer.h"
#include "BuildIR.h"

using namespace vISA;

#define FIX_OWORD_SEND_EXEC_SIZE(BLOCK_SIZE)                                   \
  (((BLOCK_SIZE) > 2) ? 16 : (BLOCK_SIZE * 4))

static uint32_t buildDescForScatter(uint32_t msgType,
                                    VISA_SVM_Block_Num numBlocks,
                                    MDC_SM2 simdMode) {
  uint32_t MD = (msgType & 0x1F) << 14;
  MD |= numBlocks << 10;
  MD |= 1 << 9;
  MD |= simdMode << 8;
  return MD;
}

bool IR_Builder::isMessageHeaderOptional(G4_Operand *surface,
                                         G4_Operand *Offset) const {
  // Message header is require for T255 stateless surface on pre-SKL devices
  // as a workaround for HW issue.
  if (needsA32MsgHeader() && isStatelessSurface(surface)) {
    return false;
  }

  // Message Header is optional when offset is 0.
  // When GlobalOffset is 0, message header is optional.
  // "If the header is not present, behavior is as if the message was sent
  // with all fields in the header set to zero."
  return Offset->isImm() && Offset->asImm()->isZero();
}

int IR_Builder::translateVISAQWGatherInst(
    VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask, G4_Predicate *pred,
    VISA_SVM_Block_Num numBlocks, G4_SrcRegRegion *surface,
    G4_SrcRegRegion *addresses, G4_DstRegRegion *dst) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  VISA_Exec_Size instExecSize = execSize;
  execSize = roundUpExecSize(execSize);

  unsigned exSize = Get_VISA_Exec_Size(execSize);
  G4_ExecSize instExSize = G4_ExecSize(Get_VISA_Exec_Size(instExecSize));
  unsigned int instOpt = Get_Gen4_Emask(eMask, instExSize);
  uint32_t messageLength = (exSize / 8);
  uint32_t responseLength =
      Get_Common_ISA_SVM_Block_Num(numBlocks) * 2 * (exSize / 8);

  uint32_t desc = buildDescForScatter(
      DC_QWORD_SCATTERED_READ, numBlocks,
      (execSize == EXEC_SIZE_8 ? MDC_SM2_SIMD8 : MDC_SM2_SIMD16));

  createSendInst(pred, dst, addresses, messageLength, responseLength,
                 instExSize, desc, SFID::DP_DC0, false, SendAccess::READ_ONLY,
                 surface, nullptr, instOpt, false);

  return VISA_SUCCESS;
}

int IR_Builder::translateVISAQWScatterInst(
    VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask, G4_Predicate *pred,
    VISA_SVM_Block_Num numBlocks, G4_SrcRegRegion *surface,
    G4_SrcRegRegion *addresses, G4_SrcRegRegion *src) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  VISA_Exec_Size instExecSize = execSize;
  execSize = roundUpExecSize(execSize);

  G4_ExecSize exSize = toExecSize(execSize);
  G4_ExecSize instExSize = toExecSize(instExecSize);
  unsigned int instOpt = Get_Gen4_Emask(eMask, instExSize);
  bool useSplitSend = useSends();

  PayloadSource sources[2]; // Maximal 2 sources, offsets + src
  unsigned len = 0;

  sources[len].opnd = addresses;
  sources[len].numElts = exSize;
  sources[len].instOpt = instOpt;
  ++len;

  unsigned numElems = Get_Common_ISA_SVM_Block_Num(numBlocks);

  sources[len].opnd = src;
  sources[len].numElts = exSize * numElems;
  sources[len].instOpt = instOpt;
  ++len;

  G4_SrcRegRegion *msgs[2]{0, 0};
  unsigned sizes[2]{0, 0};
  // For send that has smaller execsize than exSize, like
  //     "send (4)  ..."
  // Make sure to use send's execsize (4) as batchsize, not 8/16/32.
  // Thus, batchsize is min(exSize, instExSize).
  preparePayload(msgs, sizes, std::min(exSize, instExSize), useSplitSend,
                 sources, len);

  uint32_t desc = buildDescForScatter(DC_QWORD_SCATTERED_WRITE, numBlocks,
                                      execSize == EXEC_SIZE_8 ? MDC_SM2_SIMD8
                                                              : MDC_SM2_SIMD16);

  G4_DstRegRegion *dst = createNullDst(Type_UD);
  if (msgs[1] == 0) {
    vISA_ASSERT(sizes[1] == 0,
                "Expect the 2nd part of the payload has zero size!");
    createSendInst(pred, dst, msgs[0], sizes[0], 0, instExSize, desc,
                   SFID::DP_DC0, false, SendAccess::WRITE_ONLY, surface,
                   nullptr, instOpt, false);
  } else {
    createSplitSendInst(pred, dst, msgs[0], sizes[0], msgs[1], sizes[1], 0,
                        instExSize, desc, SFID::DP_DC0, false,
                        SendAccess::WRITE_ONLY, surface, nullptr, instOpt,
                        false);
  }

  return VISA_SUCCESS;
}

static void BuildStatelessSurfaceMessageHeader(IR_Builder *IRB,
                                               G4_Declare *Header) {
  // No need to mask fft id when scratch surface is bindless as
  // A32 accesses are guaranteed to not be scratch accesses.
  if (IRB->hasScratchSurface()) {
    // Clear header
    // Rx (8) = 0
    auto DstOpnd = IRB->createDst(Header->getRegVar(), 0, 0, 1, Type_UD);
    auto SrcImm0 = IRB->createImm(0, Type_UD);
    IRB->createMov(g4::SIMD8, DstOpnd, SrcImm0, InstOpt_WriteEnable, true);
    return;
  }
  // For A32, clearing off scratch space offset or Buffer Base Address is
  // always required once header is present.
  G4_Type ElemTy = Header->getElemType();

  // R0.5<31:10> is defined as Scratch Space Offset.
  // R0.5<8:0> is defined as FF Thread ID (FFTID) in SKL+ devices.
  // R0.5<7:0> is defined as FF Thread ID (FFTID) in pre-SKL devices.
  // We increase the bit range to <9:0> to copy reserved bits as well.
  const unsigned FFTID_Mask = 0x3ff;

  // Rx.5[31:0] = 0 | R0.5[9:0]
  G4_DstRegRegion *DstOpnd =
      IRB->createDst(Header->getRegVar(), 0, 5, 1, ElemTy);
  // R0.5
  G4_SrcRegRegion *SrcOpnd = IRB->createSrc(IRB->getBuiltinR0()->getRegVar(), 0,
                                            5, IRB->getRegionScalar(), ElemTy);
  // Mask
  G4_Imm *Mask = IRB->createImm(FFTID_Mask, Type_UD);
  IRB->createBinOp(G4_and, g4::SIMD1, DstOpnd, SrcOpnd, Mask,
                   InstOpt_WriteEnable, true);
}

// TODO: remove
#define SET_DATAPORT_MESSAGE_TYPE(dest, value) dest |= value << 14;

uint32_t IR_Builder::setOwordForDesc(uint32_t desc, int numOword,
                                     bool isSLM) const {
  static const uint32_t MESSAGE_SPECIFIC_CONTROL = 8;
  switch (numOword) {
  case 1:
    return desc;
  case 2:
    return desc | (0x2 << MESSAGE_SPECIFIC_CONTROL);
  case 4:
    return desc | (0x3 << MESSAGE_SPECIFIC_CONTROL);
  case 8:
    return desc | (0x4 << MESSAGE_SPECIFIC_CONTROL);
  case 16:
    vISA_ASSERT_INPUT(isSLM && has16OWordSLMBlockRW(),
           "16OWord block r/w not supported");
    return desc | (0x5 << MESSAGE_SPECIFIC_CONTROL);
  default:
    /// TODO(move to verifier): default: vISA_ASSERT_INPUT(false, "OWord block size
    /// must be 1/2/4/8.");
    return desc;
  }
}

/*
 * Translates OWord Block read CISA inst.
 *
 * For GT, assume size is 8 then the code should look like
 *
 * .declare  VX Base=m ElementSize=4 Type=ud Total=8
 * .declare  VY Base=r ElementSize=4 Type=ud Total=8
 *
 * mov  (8)     VX(0,0)<1>,  r0:ud
 * mov  (1)     VX(0,2)<1>,  P
 * send (8)     VY(0,0)<1>,  VX(0,0),    0x5,  0x02180200
 * mov  (8)     v(0,0)<1>,   VY(0,0)
 *
 * P: M0.2 in the message header (Global offset)
 *
 * 0x5 == 0 (Not the EOT)
 *
 * 0x02180200 == Bit 31-29: 000 (Reserved)
 *               Bit 28-25: 0001 (Msg. leng. = 1)
 *               Bit 24-20: 00001 (Response msg. leng. = 1)
 *               Bit 19:    1 (Header present)
 *               Bit 18:    0 (Ignored)
 *               Bit 17:    0 (Send write commit message; ignored for read
 * message Bit 16-13: 0000 (Msg. type = OWord block read - for Render Cache) Bit
 * 12-8:  00010 (Block size = 2 OWords) - can only be 1/2/4/8 for sampler/render
 * cache Bit 7-0:   00000000 + I (Binding table index)
 *
 */
int IR_Builder::translateVISAOwordLoadInst(ISA_Opcode opcode, bool modified,
                                           G4_Operand *surface,
                                           VISA_Oword_Num size,
                                           G4_Operand *offOpnd,
                                           G4_DstRegRegion *dstOpnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  unsigned num_oword = Get_VISA_Oword_Num(size);
  bool unaligned = (opcode == ISA_OWORD_LD_UNALIGNED);

  // create dcl for VX
  G4_Declare *dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);

  if (isStatelessSurface(surface)) {
    // Build stateless surface message header.
    BuildStatelessSurfaceMessageHeader(this, dcl);
  }

  /* mov (1)      VX(0,2)<1>,    P  */
  if (unaligned && (kernel.major_version == 3 && kernel.minor_version <= 1)) {
    // for vISA3.1 and earlier
    // the offset for unaligned OW load is in unit of DW, tranlate it into BYTE.
    if (offOpnd->isImm()) {
      // imm type must be UD as the result of shift could overflow word type
      G4_Imm *new_src_opnd1 =
          createImm(offOpnd->asImm()->getInt() << 2, Type_UD);
      createMovInst(dcl, 0, 2, g4::SIMD1, NULL, NULL, new_src_opnd1, true);
    } else {
      G4_DstRegRegion *dstOpnd =
          createDst(dcl->getRegVar(), 0, 2, 1, dcl->getElemType());
      createBinOp(G4_shl, g4::SIMD1, dstOpnd, offOpnd, createImm(2, Type_UW),
                  InstOpt_WriteEnable, true);
    }
  } else {
    dcl->setCapableOfReuse();
    createMovInst(dcl, 0, 2, g4::SIMD1, NULL, NULL, offOpnd, true);
  }
  // send's operands preparation
  G4_SrcRegRegion *payload = createSrcRegRegion(dcl, getRegionStride1());
  G4_DstRegRegion *d = checkSendDst(dstOpnd->asDstRegRegion());

  uint32_t temp = 0;

  if (unaligned) {
    SET_DATAPORT_MESSAGE_TYPE(temp, DC_ALIGNED_OWORD_BLOCK_READ)
  }

  // Set bit 12-8 for the message descriptor
  temp = setOwordForDesc(temp, num_oword, IsSLMSurface(surface));

  // !!!WHY???
  if (num_oword > 2) {
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

  SFID tf_id = SFID::DP_DC0;

  G4_ExecSize send_exec_size = G4_ExecSize(FIX_OWORD_SEND_EXEC_SIZE(num_oword));
  bool forceSplitSend = shouldForceSplitSend(surface);

  if (!forceSplitSend) {
    createSendInst(NULL, d, payload, 1,
                   (num_oword * 16 + getGRFSize() - 1) / getGRFSize(),
                   send_exec_size, temp, tf_id, true, SendAccess::READ_ONLY,
                   surface, NULL, InstOpt_WriteEnable, false);
  } else {
    G4_SrcRegRegion *m0 = createSrcRegRegion(dcl, getRegionStride1());
    createSplitSendInst(NULL, d, m0, 1, createNullSrc(Type_UD), 0,
                        (num_oword * 16 + getGRFSize() - 1) / getGRFSize(),
                        send_exec_size, temp, tf_id, true,
                        SendAccess::READ_ONLY, surface, nullptr,
                        InstOpt_WriteEnable, false);
  }

  return VISA_SUCCESS;
}

/*
 * Translates OWord Block write intrinsic.
 *
 * write(I, P, vector<int, S> v)
 *
 * For GT, assume S = 8 then the code should look like
 *
 * .declare  VX Base=m ElementSize=4 Type=ud Total=16
 * .declare  VY Base=m ElementSize=4 Type=ud Total=8  ALIAS(VX,8)
 *
 * mov  (8)     VX(0,0)<1>,  r0:ud
 * mov  (8)     VY(0,0)<1>,  v       // mov  (8)     VX(1,0)<1>,  v
 * mov  (1)     VX(0,2)<2>,  P
 * send (8)     null<1>,  VX(0,0),  0x5,   0x04090200
 *
 * P: M0.2 in the message header (Global offset)
 *
 * 0x5 == 0 (Not the EOT)
 *        0101 (Target Function ID: DP Render Cache)
 *
 * 0x04090200 == Bit 31-29: 000 (Reserved)
 *               Bit 28-25: 0010 (Msg. leng. = 2)
 *               Bit 24-20: 00000 (Response msg. leng. = 0)
 *               Bit 19:    1 (Header present)
 *               Bit 18:    0 (Ignored)
 *               Bit 17:    0 (Send write commit message
 *               Bit 16-13: 1000 (Msg. type = OWord block read - for Render
 * Cache) Bit 12-8:  00010 (Block size = 2 OWords) - can only be 1/2/4/8 for
 * sampler/render cache Bit 7-0:   00000000 + I (Binding table index)
 *
 */
int IR_Builder::translateVISAOwordStoreInst(G4_Operand *surface,
                                            VISA_Oword_Num size,
                                            G4_Operand *offOpnd,
                                            G4_SrcRegRegion *srcOpnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  unsigned num_oword = Get_VISA_Oword_Num(size);
  unsigned obj_size = num_oword * 16; // size of obj in bytes

  unsigned funcCtrl = DC_OWORD_BLOCK_WRITE << 14;

  uint32_t payloadGRFSize = (num_oword * 16 + getGRFSize() - 1) / getGRFSize();

  // Set bit 12-8 for the message descriptor
  funcCtrl = setOwordForDesc(funcCtrl, num_oword, IsSLMSurface(surface));
  bool forceSplitSend = shouldForceSplitSend(surface);
  if (forceSplitSend || useSends()) {
    G4_Declare *headerDcl =
        createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);

    if (isStatelessSurface(surface)) {
      // Build stateless surface message header.
      BuildStatelessSurfaceMessageHeader(this, headerDcl);
    }

    /* mov (1)     VX(0,2)<1>,   P  */
    createMovInst(headerDcl, 0, 2, g4::SIMD1, nullptr, nullptr, offOpnd, true);

    unsigned msgDesc = funcCtrl;
    unsigned extMsgLength = payloadGRFSize;
    uint16_t extFuncCtrl = 0;

    // message length = 1, response length = 0, header present = 1
    msgDesc += (1 << getSendMsgLengthBitOffset()) +
               (1 << getSendHeaderPresentBitOffset());

    G4_SendDescRaw *desc =
        createSendMsgDesc(msgDesc, 0, 1, SFID::DP_DC0, extMsgLength,
                          extFuncCtrl, SendAccess::WRITE_ONLY, surface);

    G4_ExecSize sendSize = G4_ExecSize(FIX_OWORD_SEND_EXEC_SIZE(num_oword));

    G4_SrcRegRegion *src0 = createSrcRegRegion(headerDcl, getRegionStride1());
    G4_DstRegRegion *dst = createNullDst(sendSize > 8 ? Type_UW : Type_UD);

    createSplitSendInst(nullptr, dst, src0, srcOpnd, sendSize, desc,
                        InstOpt_WriteEnable, false);
  } else {
    uint32_t temp = obj_size / TypeSize(Type_UD) + getGenxDataportIOSize();

    G4_Declare *dcl = createSendPayloadDcl(temp, Type_UD);

    /* mov  (c*r)    VX(1,0)<1>,  V */
    temp = obj_size / TypeSize(Type_UD);

    createMovSendSrcInst(dcl, 1, 0, temp, srcOpnd, InstOpt_WriteEnable);

    if (isStatelessSurface(surface)) {
      // Build stateless surface message header.
      BuildStatelessSurfaceMessageHeader(this, dcl);
    } else {
      // Copy R0 header.
      createMovR0Inst(dcl, 0, 0, true);
    }

    /* mov (1)     VX(0,2)<1>,   P  */
    createMovInst(dcl, 0, 2, g4::SIMD1, NULL, NULL, offOpnd, true);

    // send's operands preparation
    /* Size of whole operand in UINT elements */
    G4_SrcRegRegion *payload = createSrcRegRegion(dcl, getRegionStride1());

    unsigned send_size = FIX_OWORD_SEND_EXEC_SIZE(num_oword);
    G4_DstRegRegion *post_dst_opnd =
        createNullDst(send_size > 8 ? Type_UW : Type_UD);

    createSendInst(NULL, post_dst_opnd, payload, payloadGRFSize + 1, 0,
                   G4_ExecSize(send_size), funcCtrl, SFID::DP_DC0, true,
                   SendAccess::WRITE_ONLY, surface, NULL, InstOpt_WriteEnable,
                   false);
  }

  return VISA_SUCCESS;
}

static const uint8_t mapExecSizeToNumElts[6] = {1, 2, 4, 8, 16, 32};

/*
 * Translates scattered read intrinsic.
 *
 * For GT, assume N = 8 then the code should look like
 *
 * .declare  VX Base=m ElementSize=4 Type=ud Total=16
 * .declare  VY Base=r ElementSize=4 Type=ud Total=8
 *
 * mov  (8)     VX(0,0)<1>,  r0:ud
 * mov  (1)     VX(0,2)<1>,  P
 * mov  (8)     VX(1,0)<1>,  E
 * send (8)     VY(0,0)<1>,  VX(0,0),    0x5,  0x0418C200
 *
 * P: M0.2 in the message header (Global offset)
 * E: M1 in the message payload (Element offsets)
 * 0x5 == 0 (Not the EOT)
 *        0101 (Target Function ID: DP Render Cache)
 *
 * 0x0418C200 == Bit 31-29: 000 (Reserved)
 *               Bit 28-25: 0010 (Msg. leng. = 2)
 *               Bit 24-20: 00001 (Response msg. leng. = 1)
 *               Bit 19:    1 (Header present)
 *               Bit 18:    0 (Ignored)
 *               Bit 17:    0 (Send write commit message; ignored for read
 * message Bit 16-13: 0110 (Msg. type = DWord Scattered read - for Render Cache)
 *               Bit 12-10: 010 Specifies the data size for each slot. 0: 1
 * byte; 1: 2 bytes; 2: 4 bytes; 3: Reserved Bit 9-8:  00 (Block size = 8
 * DWords) Bit 7-0:   00000000 + I (Binding table index)
 *
 */
int IR_Builder::translateVISAGatherInst(
    VISA_EMask_Ctrl emask, bool modified, GATHER_SCATTER_ELEMENT_SIZE eltSize,
    VISA_Exec_Size executionSize, G4_Operand *surface, G4_Operand *gOffOpnd,
    G4_SrcRegRegion *eltOffOpnd, G4_DstRegRegion *dstOpnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  // Before GEN10, we translate DWORD GATHER on SLM to untyped GATHER4 on
  // SLM with only R channel enabled. The later is considered more
  // efficient without recalculating offsets in BYTE.
  if (eltSize == GATHER_SCATTER_DWORD && IsSLMSurface(surface)) {
    return translateVISAGather4Inst(
        emask, modified, ChannelMask::createFromAPI(CHANNEL_MASK_R),
        executionSize, surface, gOffOpnd, eltOffOpnd, dstOpnd);
  }

  G4_ExecSize exsize = G4_ExecSize(Get_VISA_Exec_Size(executionSize));
  unsigned int instOpt = Get_Gen4_Emask(emask, exsize);
  bool headerLess = isMessageHeaderOptional(surface, gOffOpnd);
  // Element size in gather/scatter message. Initially, we assume it's the
  // same as the request.
  GATHER_SCATTER_ELEMENT_SIZE msgEltSize = eltSize;

  // SLM access
  //              HEADLESS    BYTE    WORD    DWORD
  // BDW          Opt         YES     NO      NO
  // SKL          Req         YES     NO      NO
  // CNL          Req         YES     NO      YES

  G4_Predicate *pred = NULL; // for SIMD1 gather
  uint8_t numElt = mapExecSizeToNumElts[executionSize];
  // we need to treat simd1 as simd8 in several places during code gen
  uint8_t effectiveNumElt = (numElt == 1 ? 8 : numElt);

  if (!headerLess && noSLMMsgHeader() && IsSLMSurface(surface)) {
    // From SKL, SLM messages forbid message header. Recalculate offset by
    // adding global offset and force headerLess.
    G4_Declare *dcl = createSendPayloadDcl(numElt, eltOffOpnd->getType());
    dcl->setSubRegAlign(getGRFAlign());
    G4_DstRegRegion *newEltOffOpnd = createDstRegRegion(dcl, 1);
    createBinOp(G4_add, G4_ExecSize(numElt), newEltOffOpnd, eltOffOpnd,
                gOffOpnd, instOpt, true);
    eltOffOpnd = createSrcRegRegion(dcl, numElt == 1 ? getRegionScalar()
                                                     : getRegionStride1());
    headerLess = true;
  }

  bool useSplitSend = useSends();
  // When header is not required, split-send is not needed as there's only
  // one part in the message. When header is present, we will split the
  // message as (header, offset).
  if (headerLess)
    useSplitSend = false;

  G4_Declare *header = 0;
  G4_Declare *offset = createSendPayloadDcl(numElt, Type_UD);
  offset->setSubRegAlign(getGRFAlign());

  if (useSplitSend) {
    vISA_ASSERT(!headerLess,
                "SplitSend should not be used when header is not required!");
    // Without header, it's unnecessary to split the message.
    header = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);
  } else if (!headerLess) {
    header = createSendPayloadDcl(getGenxDataportIOSize() + effectiveNumElt,
                                  Type_UD);
    offset->setAliasDeclare(header, numEltPerGRF<Type_UB>());
  }

  G4_SrcRegRegion *msgSrcOpnd = NULL;

  if (headerLess) {
    vISA_ASSERT(
        !header,
        "'header' should not be allocated when header is not required!");

    if (eltSize == GATHER_SCATTER_WORD ||
        (eltSize != GATHER_SCATTER_BYTE && IsSLMSurface(surface))) {
      // Use byte gather for WORD gather as well as SLM surfaces (only supports
      // byte gather) need a shift to make the offset to be byte offset shl (8)
      // tmp<1>:ud elt_off<8;8,1>:ud 0x2:uw Don't do this for Dword because we
      // use the dword scatter message instead
      G4_DstRegRegion *tmpDstOpnd = createDstRegRegion(offset, 1);
      createBinOp(G4_shl, G4_ExecSize(numElt), tmpDstOpnd, eltOffOpnd,
                  createImm(unsigned(eltSize), Type_UD), instOpt, true);
      msgSrcOpnd = createSrcRegRegion(offset, getRegionStride1());
      msgEltSize = GATHER_SCATTER_BYTE;
    } else {
      msgSrcOpnd = eltOffOpnd;
    }
  } else {
    if (isStatelessSurface(surface)) {
      // Build stateless surface message header.
      BuildStatelessSurfaceMessageHeader(this, header);
    } else {
      // Copy R0 header.
      createMovR0Inst(header, 0, 0, true);
    }

    G4_DstRegRegion *dst1_opnd =
        createDst(offset->getRegVar(), 0, 0, 1, offset->getElemType());

    if (eltSize == GATHER_SCATTER_WORD || IsSLMSurface(surface)) {
      // For non-SLM surface, WORD gather/scatter has no hardware
      // support and must be translated into BYTE gather/scatter.
      //
      // SLM surface supports only BYTE gather/scatter
      // support and also needs translating into BYTE gather/scatter.
      //
      /* mov (1)     VX(0,2)<1>,   P  */
      if (gOffOpnd->isImm()) {
        G4_Imm *new_src_opnd1 =
            createImm(gOffOpnd->asImm()->getInt() *
                          (eltSize == GATHER_SCATTER_WORD ? 2 : 4),
                      gOffOpnd->getType());
        createMovInst(header, 0, 2, g4::SIMD1, NULL, NULL, new_src_opnd1, true);
      } else {
        G4_DstRegRegion *dst2_opnd =
            createDst(header->getRegVar(), 0, 2, 1, header->getElemType());

        createBinOp(G4_shl, g4::SIMD1, dst2_opnd, gOffOpnd,
                    createImm((unsigned)eltSize, Type_UD), InstOpt_WriteEnable,
                    true);
      }
      createBinOp(G4_shl, G4_ExecSize(numElt), dst1_opnd, eltOffOpnd,
                  createImm((unsigned)eltSize, Type_UD), instOpt, true);
      msgEltSize = GATHER_SCATTER_BYTE;
    } else {
      /* mov (1)     VX(0,2)<1>,   P  */
      createMovInst(header, 0, 2, g4::SIMD1, NULL, NULL, gOffOpnd, true);
      /* mov  (numElt)    VX(1,0)<1>,  E */
      createMov(G4_ExecSize(numElt), dst1_opnd, eltOffOpnd, instOpt, true);
    }

    // Create a <8;8,1> src region for the send payload
    msgSrcOpnd = createSrcRegRegion(header, getRegionStride1());
  }

  G4_DstRegRegion *d = dstOpnd->asDstRegRegion();

  SFID tf_id = SFID::DP_DC0;
  unsigned temp = 0;
  // Set bit 9-8 for the message descriptor
  if (msgEltSize == GATHER_SCATTER_DWORD) {
    if (effectiveNumElt == 8) {
      temp += 2 << 8;
    } else {
      temp += 3 << 8;
    }
    temp += DC_DWORD_SCATTERED_READ << 14; // '0011' for DWORD scattered read
  } else {
    if (effectiveNumElt == 16) {
      temp += 1 << 8;
    }
    temp += (unsigned char)eltSize << 10;
    temp += DC_BYTE_SCATTERED_READ << 14;
  }

  if (useSplitSend) {
    vISA_ASSERT(!headerLess,
                "SplitSend should only be used when header is required!");

    G4_SrcRegRegion *m0 = createSrcRegRegion(header, getRegionStride1());
    G4_SrcRegRegion *m1 = createSrcRegRegion(offset, getRegionStride1());
    createSplitSendInst(
        pred, d, m0, 1, m1, effectiveNumElt / getGenxDataportIOSize(),
        effectiveNumElt / getGenxDataportIOSize(), G4_ExecSize(numElt), temp,
        tf_id, true, SendAccess::READ_ONLY, surface, NULL, instOpt, false);
  } else {
    createSendInst(pred, d, msgSrcOpnd,
                   headerLess ? effectiveNumElt / getGenxDataportIOSize()
                              : effectiveNumElt / getGenxDataportIOSize() + 1,
                   effectiveNumElt / getGenxDataportIOSize(),
                   G4_ExecSize(numElt), temp, tf_id, !headerLess,
                   SendAccess::READ_ONLY, surface, nullptr, instOpt, false);
  }

  return VISA_SUCCESS;
}

/*
 * Translates scattered write intrinsic.
 *
 * For GT, assume N = 8 then the code should look like
 *
 * .declare  VX Base=m ElementSize=4 Type=ud Total=24
 *
 * mov  (8)     VX(0,0)<1>,  r0:ud
 * mov  (1)     VX(0,2)<1>,  P
 * mov  (8)     VX(1,0)<1>,  E
 * mov  (8)     VX(2,0)<1>,  V
 * send (8)     null<1>,     VX(0,0),    0x5,  0x06096200
 *
 * P: M0.2 in the message header (Global offset)
 * E: M1 in the message payload (Element offsets)
 * v: M2 in the message payload (written data)
 *
 * 0x5 == 0 (Not the EOT)
 *        0101 (Target Function ID: DP Render Cache)
 *
 * 0x06096200 == Bit 31-29: 000 (Reserved)
 *               Bit 28-25: 0011 (Msg. leng. = 3)
 *               Bit 24-20: 00000 (Response msg. leng. = 0)
 *               Bit 19:    1 (Header present)
 *               Bit 18:    0 (Ignored)
 *               Bit 17:    0 (Send write commit message)
 *               Bit 16-13: 1011 (Msg. type = DWord Scattered write - for Render
 * Cache) Bit 12-8:  00010 (Block size = 8 DWords) Bit 7-0:   00000000 + I
 * (Binding table index)
 *
 */
int IR_Builder::translateVISAScatterInst(
    VISA_EMask_Ctrl emask, GATHER_SCATTER_ELEMENT_SIZE eltSize,
    VISA_Exec_Size executionSize, G4_Operand *surface, G4_Operand *gOffOpnd,
    G4_SrcRegRegion *eltOffOpnd, G4_SrcRegRegion *srcOpnd) {
  // Before GEN10, we translate DWORD SCATTER on SLM to untyped GATHER4 on
  // SLM with only R channel enabled. The later is considered more
  // efficient without recalculating offsets in BYTE.
  if (eltSize == GATHER_SCATTER_DWORD && IsSLMSurface(surface)) {
    return translateVISAScatter4Inst(
        emask, ChannelMask::createFromAPI(CHANNEL_MASK_R), executionSize,
        surface, gOffOpnd, eltOffOpnd, srcOpnd);
  }

  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize exsize = G4_ExecSize(Get_VISA_Exec_Size(executionSize));
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, exsize);
  G4_Predicate *pred = NULL;
  // Element size in gather/scatter message. Initially, we assume it's the same
  // as the request.
  GATHER_SCATTER_ELEMENT_SIZE msgEltSize = eltSize;

  uint8_t numElt = mapExecSizeToNumElts[executionSize];
  // we need to treat simd1 as simd8 in several places during code gen
  uint8_t effectiveNumElt = (numElt == 1 ? 8 : numElt);

  bool headerLess = isMessageHeaderOptional(surface, gOffOpnd);
  G4_SrcRegRegion *msgSrcOpnd = NULL;

  // SLM access
  //              HEADLESS    BYTE    WORD    DWORD
  // BDW          Opt         YES     NO      NO
  // SKL          Req         YES     NO      NO
  // CNL          Req         YES     NO      YES

  if (!headerLess && noSLMMsgHeader() && IsSLMSurface(surface)) {
    // From SKL, SLM messages forbid message header. Recalculate offset by
    // adding global offset and force headerLess.
    G4_Declare *dcl = createSendPayloadDcl(numElt, eltOffOpnd->getType());
    G4_DstRegRegion *newEltOffOpnd = createDstRegRegion(dcl, 1);
    createBinOp(G4_add, G4_ExecSize(numElt), newEltOffOpnd, eltOffOpnd,
                gOffOpnd, instOpt, true);
    eltOffOpnd = createSrcRegRegion(dcl, numElt == 1 ? getRegionScalar()
                                                     : getRegionStride1());
    headerLess = true;
  }

  if (headerLess) {
    // header size = 2 * #elt
    G4_Declare *dcl = createSendPayloadDcl(effectiveNumElt * 2, Type_UD);
    G4_DstRegRegion *tmpDstOpnd = createDstRegRegion(dcl, 1);
    if (eltSize == GATHER_SCATTER_WORD ||
        (eltSize != GATHER_SCATTER_BYTE && IsSLMSurface(surface))) {
      // For non-SLM surface,
      // need a shift to make the offset to be byte offset
      // shl (esize) tmp.0<1>:ud elt_off<8;8,1>:ud 0x2:uw
      // Don't do this for Dword because we use the dword scatter message
      // instead
      //
      // SLM surface has only BYTE scattered
      // read/write support. Always use BYTE scater.
      createBinOp(G4_shl, G4_ExecSize(numElt), tmpDstOpnd, eltOffOpnd,
                  createImm(unsigned(eltSize), Type_UD), instOpt, true);
      msgEltSize = GATHER_SCATTER_BYTE;
    } else {
      createMov(G4_ExecSize(numElt), tmpDstOpnd, eltOffOpnd, instOpt, true);
    }

    createMovSendSrcInst(dcl, effectiveNumElt / 8, 0, numElt, srcOpnd, instOpt);
    msgSrcOpnd = createSrcRegRegion(dcl, getRegionStride1());
  } else {
    // mov (8)      VX(0,0)<1>,  r0:ud
    // add dcl for VX
    G4_Declare *dcl = createSendPayloadDcl(
        getGenxDataportIOSize() + effectiveNumElt * 2, Type_UD);

    if (isStatelessSurface(surface)) {
      // Build stateless surface message header.
      BuildStatelessSurfaceMessageHeader(this, dcl);
    } else {
      // Copy R0 header.
      createMovR0Inst(dcl, 0, 0, true);
    }

    auto dst1_opnd = createDst(dcl->getRegVar(), 1, 0, 1, dcl->getElemType());

    if (eltSize == GATHER_SCATTER_WORD || IsSLMSurface(surface)) {
      // For non-SLM surface, WORD gather/scatter has no hardware
      // supportr and must be translated into BYTE gather/scatter.
      //
      // For SLM surface, gen9 devices has only BYTE gather/scatter
      // support and also needs translating into BYTE gather/scatter.
      //
      /* mov (1)     VX(0,2)<1>,   P  */
      if (gOffOpnd->isImm()) {
        G4_Imm *new_src_opnd1 =
            createImm(gOffOpnd->asImm()->getInt() *
                          (eltSize == GATHER_SCATTER_WORD ? 2 : 4),
                      gOffOpnd->getType());
        createMovInst(dcl, 0, 2, g4::SIMD1, NULL, NULL, new_src_opnd1, true);
      } else {
        G4_DstRegRegion *dst2_opnd =
            createDst(dcl->getRegVar(), 0, 2, 1, dcl->getElemType());
        createBinOp(G4_shl, g4::SIMD1, dst2_opnd, gOffOpnd,
                    createImm((unsigned)eltSize, Type_UD), InstOpt_WriteEnable,
                    true);
      }
      createBinOp(G4_shl, G4_ExecSize(numElt), dst1_opnd, eltOffOpnd,
                  createImm((unsigned)eltSize, Type_UD), instOpt, true);
      msgEltSize = GATHER_SCATTER_BYTE;
    } else {
      /* mov (1)     VX(0,2)<1>,   P  */
      createMovInst(dcl, 0, 2, g4::SIMD1, NULL, NULL, gOffOpnd, true);
      /* mov  (numElt)    VX(1,0)<1>,  E */
      createMov(G4_ExecSize(numElt), dst1_opnd, eltOffOpnd, instOpt, true);
    }

    /* mov  (numElt)    VX(numElt/8+1,0)<1>,  V */
    createMovSendSrcInst(dcl, (effectiveNumElt / 8 + 1), 0, numElt, srcOpnd,
                         instOpt);

    // send's operands preparation
    // create a currDst for VX
    msgSrcOpnd = createSrcRegRegion(dcl, getRegionStride1());
  }

  unsigned temp = 0;

  // Set bit 9-8 for the message descriptor
  if (msgEltSize == GATHER_SCATTER_DWORD) {
    if (effectiveNumElt == 8) {
      temp += 2 << 8;
    } else {
      temp += 3 << 8;
    }
    temp += DC_DWORD_SCATTERED_WRITE << 14;
  } else {
    if (effectiveNumElt == 16) {
      temp += 1 << 8;
    }
    temp += (unsigned char)eltSize << 10;
    temp += DC_BYTE_SCATTERED_WRITE << 14;
  }

  G4_DstRegRegion *post_dst_opnd =
      createNullDst(effectiveNumElt > 8 ? Type_UW : Type_UD);

  createSendInst(pred, post_dst_opnd, msgSrcOpnd,
                 headerLess ? effectiveNumElt / getGenxDataportIOSize() * 2
                            : effectiveNumElt / getGenxDataportIOSize() * 2 + 1,
                 0, G4_ExecSize(numElt), temp, SFID::DP_DC0, !headerLess,
                 SendAccess::WRITE_ONLY, surface, NULL, instOpt, false);

  return VISA_SUCCESS;
}

static void BuildUntypedStatelessSurfaceMessageHeader(IR_Builder *IRB,
                                                      G4_Declare *Header) {
  // Set PSM (Pixel Sample Mask) in MH1_A32_PSM
  G4_Type ElemTy = Header->getElemType();

  // R0.7<31:0> is defined as MHC_PSM where the lower 16 bits specify the
  // pixel sample mask.
  const unsigned PSM_Mask = 0xffff;

  // Rx.7[31:0] = 0xFFFF
  G4_DstRegRegion *DstOpnd =
      IRB->createDst(Header->getRegVar(), 0, 7, 1, ElemTy);
  // Mask
  G4_Imm *Mask = IRB->createImm(PSM_Mask, Type_UD);
  IRB->createMov(g4::SIMD1, DstOpnd, Mask, InstOpt_WriteEnable, true);

  BuildStatelessSurfaceMessageHeader(IRB, Header);
}

/*
 * Translates untyped surface read.
 *
 * For GT, assume N = 8 then the code should look like
 *
 * .declare  VX Base=m ElementSize=4 Type=ud Total=16
 * .declare  VY Base=r ElementSize=4 Type=ud Total=8
 *
 * mov  (8)     VX(0,0)<1>,  r0:ud
 * mov  (8)     VX(1,0)<1>,  P+E
 * send (8)     VY(0,0)<1>,  VX(0,0),    0x5,  0x0418C200
 *
 * E: M1 in the message payload (Element offsets in BYTEs)
 * 1010 (Target Function ID: Data Cache)
 *
 * 0x0418C200 == Bit 31-29: 000 (Reserved)
 *               Bit 28-25: 0010 (Msg. leng. = 2)
 *               Bit 24-20: 00001 (Response msg. leng. = 1)
 *               Bit 19:    1 (Header present)
 *               Bit 18:    0 (Ignored)
 *               Bit 17-14: 1101 (Msg. type = untyped write - for data Cache)
 *               Bit 13-12:  0010 (SIMD mode = 8)
 *               Bit 11-8:  0000 (masked channels)
 *               Bit 7-0:   00000000 + I (Binding table index)
 *
 */
int IR_Builder::translateVISAGather4Inst(
    VISA_EMask_Ctrl emask, bool modified, ChannelMask chMask,
    VISA_Exec_Size executionSize, G4_Operand *surface, G4_Operand *gOffOpnd,
    G4_SrcRegRegion *eltOffOpnd, G4_DstRegRegion *dstOpnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize exsize = G4_ExecSize(Get_VISA_Exec_Size(executionSize));
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, exsize);
  unsigned int num_channel = chMask.getNumEnabledChannels();

  uint8_t numElt = mapExecSizeToNumElts[executionSize];
  uint8_t hdrSize = 0;

  bool useSplitSend = useSends();

  G4_Declare *header = 0;
  G4_Declare *offset = createSendPayloadDcl(numElt, Type_UD);

  if (surface && isStatelessSurface(surface) && needsA32MsgHeader()) {
    // Header is required to work around a HW issue on pre-SKL devices.
    hdrSize = getGenxDataportIOSize();
    if (useSplitSend) {
      header = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);
    } else {
      header = createSendPayloadDcl(getGenxDataportIOSize() + numElt, Type_UD);
      offset->setAliasDeclare(header, numEltPerGRF<Type_UB>());
    }
  } else {
    // When the surface is not stateless one, header is not used and therefore
    // split-send is not used.
    useSplitSend = false;
  }

  if (header) {
    // With 'header' allocated, we need prepare the header for the
    // (stateless) surface.
    vISA_ASSERT(isStatelessSurface(surface),
                "With 'header' allocated, stateless surface is expected!");
    // Build stateless surface message header.
    BuildUntypedStatelessSurfaceMessageHeader(this, header);
  }

  // convert to byte address
  // shl (esize) offset<1>:ud elt_off<8;8,1>:ud 2:uw
  G4_DstRegRegion *dst1_opnd =
      createDst(offset->getRegVar(), 0, 0, 1, offset->getElemType());

  G4_Declare *tmp_dcl = createTempVar(numElt, Type_UD, getGRFAlign());
  G4_DstRegRegion *dst3_opnd =
      createDst(tmp_dcl->getRegVar(), 0, 0, 1, tmp_dcl->getElemType());

  createBinOp(G4_shl, G4_ExecSize(numElt), dst3_opnd, eltOffOpnd,
              createImm(2, Type_UW), instOpt, true);

  G4_SrcRegRegion *src2_opnd = createSrc(
      tmp_dcl->getRegVar(), 0, 0, getRegionStride1(), tmp_dcl->getElemType());

  // As untyped surface message use MH_IGNORE based header, if global offset
  // is non-zero, we need recalculate element offsets.
  if (gOffOpnd->isImm()) {
    if (gOffOpnd->asImm()->getInt() != 0) {
      gOffOpnd =
          createImm(gOffOpnd->asImm()->getInt() * 4, gOffOpnd->getType());
      createBinOp(G4_add, G4_ExecSize(numElt), dst1_opnd, src2_opnd, gOffOpnd,
                  instOpt, true);
    } else {
      createMov(G4_ExecSize(numElt), dst1_opnd, src2_opnd, instOpt, true);
    }
  } else {
    G4_Declare *tmp_dcl1 = createTempVar(1, gOffOpnd->getType(), Any);
    G4_DstRegRegion *dst2_opnd =
        createDst(tmp_dcl1->getRegVar(), 0, 0, 1, tmp_dcl1->getElemType());

    createBinOp(G4_shl, g4::SIMD1, dst2_opnd, gOffOpnd, createImm(2, Type_UW),
                InstOpt_WriteEnable, true);

    G4_SrcRegRegion *src1Opnd =
        createSrc(tmp_dcl1->getRegVar(), 0, 0, getRegionScalar(),
                  tmp_dcl1->getElemType());

    createBinOp(G4_add, G4_ExecSize(numElt), dst1_opnd, src2_opnd, src1Opnd,
                instOpt, true);
  }

  // send's operands preparation

  G4_DstRegRegion *d = checkSendDst(dstOpnd->asDstRegRegion());

  unsigned temp = 0;

  // Set bit 13-12 for the message descriptor
  if (numElt == 8) {
    temp += 2 << 12;
  } else {
    temp += 1 << 12;
  }

  SFID tf_id = SFID::DP_DC1;
  temp += DC1_UNTYPED_SURFACE_READ << 14;

  // bits 11-8: channel mask
  // HW defines 0 to mean the channel is on, so we have to flip it
  temp += chMask.getHWEncoding() << 8;

  if (surface == NULL) {
    temp |= 0xFE;
  }

  if (useSplitSend) {
    vISA_ASSERT(header,
                "'header' should be allocated when split-send is to be used.");

    G4_SrcRegRegion *m0 = createSrcRegRegion(header, getRegionStride1());
    G4_SrcRegRegion *m1 = createSrcRegRegion(offset, getRegionStride1());
    createSplitSendInst(NULL, d, m0, 1, m1, numElt / getGenxDataportIOSize(),
                        (numElt / getGenxDataportIOSize()) * num_channel,
                        G4_ExecSize(numElt), temp, tf_id, hdrSize != 0,
                        SendAccess::READ_ONLY, surface, NULL, instOpt, false);
  } else {
    G4_SrcRegRegion *payload =
        createSrcRegRegion(header ? header : offset, getRegionStride1());
    createSendInst(NULL, d, payload,
                   (hdrSize + numElt) / getGenxDataportIOSize(),
                   (numElt / getGenxDataportIOSize()) * num_channel,
                   G4_ExecSize(numElt), temp, tf_id, hdrSize != 0,
                   SendAccess::READ_ONLY, surface, NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

/*
 * Translates untyped surface write intrinsic.
 *
 * For GT, assume N = 8 then the code should look like
 *
 * .declare  VX Base=m ElementSize=4 Type=ud Total=24
 *
 * mov  (8)     VX(0,0)<1>,  r0:ud
 * mov  (8)     VX(1,0)<1>,  E + P
 * mov  (8)     VX(2,0)<1>,  V
 * send (8)     null<1>,     VX(0,0),    0x5,  0x06096200
 *
 * E: M1 in the message payload (Element offsets)
 * v: M2 in the message payload (written data)
 *
 * 1010 (Target Function ID: DP Data Cache)
 *
 * 0x06096200 == Bit 31-29: 000 (Reserved)
 *               Bit 28-25: 0011 (Msg. leng. = 3)
 *               Bit 24-20: 00000 (Response msg. leng. = 0)
 *               Bit 19:    1 (Header present)
 *               Bit 18:    0 (Ignored)
 *               Bit 17-14: 1101 (Msg. type = untyped write - for data Cache)
 *               Bit 13-12:  0010 (SIMD mode = 8)
 *                  Bit 11-8:  0000 (masked channels)
 *               Bit 7-0:   00000000 + I (Binding table index)
 *
 */
int IR_Builder::translateVISAScatter4Inst(
    VISA_EMask_Ctrl emask, ChannelMask chMask, VISA_Exec_Size executionSize,
    G4_Operand *surface, G4_Operand *gOffOpnd, G4_SrcRegRegion *eltOffOpnd,
    G4_SrcRegRegion *srcOpnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize exsize = G4_ExecSize(Get_VISA_Exec_Size(executionSize));
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, exsize);

  unsigned int num_channel = chMask.getNumEnabledChannels();

  uint8_t numElt = mapExecSizeToNumElts[executionSize];
  uint8_t hdrSize = 0;

  unsigned int data_size = numElt * num_channel;
  G4_Declare *src_dcl =
      srcOpnd->asSrcRegRegion()->getBase()->asRegVar()->getDeclare();

  int payload_size = numElt + data_size;

  bool useSplitSend = useSends();

  G4_Declare *header = 0;
  G4_Declare *offset = 0;
  G4_Declare *data = createSendPayloadDcl(data_size, Type_UD);

  if (surface && isStatelessSurface(surface) && needsA32MsgHeader()) {
    // Header is required to work around a HW issue on pre-SKL devices.
    hdrSize = getGenxDataportIOSize();
    offset = createSendPayloadDcl(numElt, Type_UD);
    if (useSplitSend) {
      // When header is required, we split the message as
      // (header, offset + data) if split-send is supported.
      header = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);
      offset = createSendPayloadDcl(payload_size, Type_UD);
      data->setAliasDeclare(offset, (numElt / 8) * numEltPerGRF<Type_UB>());
    } else {
      header =
          createSendPayloadDcl(getGenxDataportIOSize() + payload_size, Type_UD);
      offset->setAliasDeclare(header, numEltPerGRF<Type_UB>());
      data->setAliasDeclare(header,
                            numEltPerGRF<Type_UB>() * ((numElt / 8) + 1));
    }
  } else {
    if (useSplitSend) {
      // When header is not required, we split the message as (offset, data)
      // if split-send is supported.
      offset = createSendPayloadDcl(numElt, Type_UD);
    } else {
      offset = createSendPayloadDcl(payload_size, Type_UD);
      data->setAliasDeclare(offset, (numElt / 8) * numEltPerGRF<Type_UB>());
    }
  }

  if (header) {
    // With 'header' allocated, we need prepare the header for the
    // (stateless) surface.
    vISA_ASSERT(isStatelessSurface(surface),
                "With 'header' allocated, stateless surface is expected!");
    // Build stateless surface message header.
    BuildUntypedStatelessSurfaceMessageHeader(this, header);
  }

  if (!header && useSplitSend) {
    data = src_dcl;
  } else {
    // Copy data from src operand.
    for (unsigned i = 0; i != num_channel; ++i) {
      G4_SrcRegRegion *s2_opnd =
          createSrc(src_dcl->getRegVar(), (i * numElt) / 8, 0,
                    getRegionStride1(), src_dcl->getElemType());
      createMovSendSrcInst(data, (i * numElt) / 8, 0, numElt, s2_opnd, instOpt);
    }
  }

  // mov  VX(0,0)<1>, r0
  // createMovR0Inst(header, 0, 0, true);

  G4_DstRegRegion *dst1_opnd =
      createDst(offset->getRegVar(), 0, 0, 1, offset->getElemType());

  G4_Declare *tmp_dcl = createTempVar(numElt, Type_UD, getGRFAlign());
  G4_DstRegRegion *dst3_opnd =
      createDst(tmp_dcl->getRegVar(), 0, 0, 1, tmp_dcl->getElemType());

  createBinOp(G4_shl, G4_ExecSize(numElt), dst3_opnd, eltOffOpnd,
              createImm(2, Type_UW), instOpt, true);

  G4_SrcRegRegion *src2_opnd = createSrc(
      tmp_dcl->getRegVar(), 0, 0, getRegionStride1(), tmp_dcl->getElemType());

  if (gOffOpnd->isImm()) {
    if (gOffOpnd->asImm()->getInt() != 0) {
      gOffOpnd =
          createImm(gOffOpnd->asImm()->getInt() * 4, gOffOpnd->getType());
      createBinOp(G4_add, G4_ExecSize(numElt), dst1_opnd, src2_opnd, gOffOpnd,
                  instOpt, true);
    } else {
      createMov(G4_ExecSize(numElt), dst1_opnd, src2_opnd, instOpt, true);
    }
  } else {
    G4_Declare *tmp_dcl1 = createTempVar(1, gOffOpnd->getType(), Any);
    G4_DstRegRegion *dst2_opnd =
        createDst(tmp_dcl1->getRegVar(), 0, 0, 1, tmp_dcl1->getElemType());

    createBinOp(G4_shl, g4::SIMD1, dst2_opnd, gOffOpnd, createImm(2, Type_UW),
                InstOpt_WriteEnable, true);

    G4_SrcRegRegion *src1Opnd =
        createSrc(tmp_dcl1->getRegVar(), 0, 0, getRegionScalar(),
                  tmp_dcl1->getElemType());

    createBinOp(G4_add, G4_ExecSize(numElt), dst1_opnd, src2_opnd, src1Opnd,
                instOpt, true);
  }

  // send's operands preparation
  unsigned temp = 0;

  // Set bit 13-12 for the message descriptor
  if (numElt == 8) {
    temp += 2 << 12;
  } else {
    temp += 1 << 12;
  }

  SFID tf_id = SFID::DP_DC1;
  temp += DC1_UNTYPED_SURFACE_WRITE << 14;
  // bits 11-8: channel mask
  temp += chMask.getHWEncoding() << 8;

  // Set bit 9-8 for the message descriptor

  if (surface == NULL) {
    temp |= 0xFF - 1;
  }

  G4_DstRegRegion *post_dst_opnd =
      createNullDst(numElt > 8 ? Type_UW : Type_UD);

  if (useSplitSend) {
    G4_SrcRegRegion *m0 = 0;
    unsigned m0Len = 0;
    G4_SrcRegRegion *m1 = 0;
    unsigned m1Len = 0;
    if (header) {
      m0 = createSrcRegRegion(header, getRegionStride1());
      m0Len = 1;
      m1 = createSrcRegRegion(offset, getRegionStride1());
      m1Len = payload_size / getGenxDataportIOSize();
    } else {
      m0 = createSrcRegRegion(offset, getRegionStride1());
      m0Len = numElt / getGenxDataportIOSize();
      m1 = createSrcRegRegion(data, getRegionStride1());
      m1Len = data_size / getGenxDataportIOSize();
    }
    createSplitSendInst(NULL, post_dst_opnd, m0, m0Len, m1, m1Len, 0,
                        G4_ExecSize(numElt), temp, tf_id, hdrSize != 0,
                        SendAccess::WRITE_ONLY, surface, NULL, instOpt, false);
  } else {
    G4_SrcRegRegion *payload =
        createSrcRegRegion(header ? header : offset, getRegionStride1());
    createSendInst(NULL, post_dst_opnd, payload,
                   (numElt * (num_channel + 1) + hdrSize) /
                       getGenxDataportIOSize(),
                   0, G4_ExecSize(numElt), temp, tf_id, hdrSize != 0,
                   SendAccess::WRITE_ONLY, surface, NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

static bool IsFloatAtomicOps(VISAAtomicOps op) {
  return op == ATOMIC_FMAX || op == ATOMIC_FMIN || op == ATOMIC_FCMPWR ||
         op == ATOMIC_FADD || op == ATOMIC_FSUB;
}

static void BuildMH1_A32_PSM(IR_Builder *IRB, G4_Declare *header) {
  // Clear header. Ignore PSM so far.
  G4_DstRegRegion *h = IRB->createDst(header->getRegVar(), 0, 0, 1, Type_UD);
  IRB->createMov(g4::SIMD8, h, IRB->createImm(0, Type_UD), InstOpt_WriteEnable,
                 true);
  // Set PSM to all 1s.
  G4_DstRegRegion *h0_7 = IRB->createDst(header->getRegVar(), 0, 7, 1, Type_UD);
  G4_Imm *Mask = IRB->createImm(0xFFFF, Type_UD);
  IRB->createMov(g4::SIMD1, h0_7, Mask, InstOpt_WriteEnable, true);
}

static void BuildMH1_BTS_PSM(IR_Builder *IRB, G4_Declare *header) {
  // Clear header
  G4_DstRegRegion *h = IRB->createDst(header->getRegVar(), 0, 0, 1, Type_UD);
  IRB->createMov(g4::SIMD8, h, IRB->createImm(0, Type_UD), InstOpt_WriteEnable,
                 true);
  // Set PSM to 0xFFFF so far.
  G4_Operand *maskImm = IRB->createImm(0xFFFF, Type_UD);
  G4_DstRegRegion *pitchDst =
      IRB->createDst(header->getRegVar(), 0, 7, 1, Type_UD);
  IRB->createMov(g4::SIMD1, pitchDst, maskImm, InstOpt_WriteEnable, true);
}

// This version takes byte offsets and predicates
int IR_Builder::translateVISADwordAtomicInst(
    VISAAtomicOps atomicOp, bool is16Bit, G4_Predicate *pred,
    VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask, G4_Operand *surface,
    G4_SrcRegRegion *offsets, G4_SrcRegRegion *src0, G4_SrcRegRegion *src1,
    G4_DstRegRegion *dst) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  vISA_ASSERT_INPUT(!IsFloatAtomicOps(atomicOp) || hasFloatAtomics(),
              "Float atomic operations are only supported on SKL+ devices");

  vISA_ASSERT_INPUT(getPlatform() >= Xe_XeHPSDV ||
                  ((atomicOp != ATOMIC_FADD) && (atomicOp != ATOMIC_FSUB)),
              "FADD/FSUB atomic operations are only supported on this devices");

  vASSERT(surface);

  VISA_Exec_Size instExecSize = execSize;
  execSize = roundUpExecSize(execSize);

  // always 8 or 16
  G4_ExecSize exSize = toExecSize(execSize);
  // can be 1 for scalar atomics
  G4_ExecSize instExSize = toExecSize(instExecSize);
  G4_InstOpts instOpt = Get_Gen4_Emask(eMask, instExSize);
  unsigned subOpc = Get_Atomic_Op(atomicOp);

  bool useSplitSend = useSends();
  bool hasRet = !dst->isNullReg();

  if (atomicOp == ATOMIC_CMPXCHG) {
    std::swap(src0, src1);
  }

  PayloadSource sources[4]; // optional header + offsets + [src0] + [src1]
  unsigned len = 0;

  bool useHeader =
      needsA32MsgHeader() && isStatelessSurface(surface);
  if (useHeader) {
    G4_Declare *dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);

    BuildMH1_A32_PSM(this, dcl);

    G4_SrcRegRegion *header = createSrcRegRegion(dcl, getRegionStride1());
    sources[len].opnd = header;
    sources[len].numElts = numEltPerGRF<Type_UD>();
    sources[len].instOpt = InstOpt_WriteEnable;
    sources[len].copyExecSize = g4::SIMD8; // header has 8 DWs
    ++len;
  }

  sources[len].opnd = offsets;
  sources[len].numElts = exSize;
  sources[len].instOpt = instOpt;
  ++len;

  if (src0 && !src0->isNullReg()) {
    sources[len].opnd = src0;
    sources[len].numElts = exSize;
    sources[len].instOpt = instOpt;
    ++len;
  }

  if (src1 && !src1->isNullReg()) {
    sources[len].opnd = src1;
    sources[len].numElts = exSize;
    sources[len].instOpt = instOpt;
    ++len;
  }

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, std::min(exSize, instExSize), useSplitSend,
                 sources, len);

  SFID sfid = SFID::DP_DC1;
  unsigned MD = 0;
  bool IsFloatOp = IsFloatAtomicOps(atomicOp);

  // Bit 12 specifies the SIMD mode.
  MD |= (execSize == EXEC_SIZE_8 ? MDC_SM2R_SIMD8 : MDC_SM2R_SIMD16) << 12;
  if (is16Bit) {
    MD |= (IsFloatOp ? static_cast<unsigned>(DC1_UNTYPED_HALF_FLOAT_ATOMIC)
                     : static_cast<unsigned>(DC1_UNTYPED_HALF_INTEGER_ATOMIC))
          << 14;
  } else {
    MD |= (IsFloatOp ? static_cast<unsigned>(DC1_UNTYPED_FLOAT_ATOMIC)
                     : static_cast<unsigned>(DC1_UNTYPED_ATOMIC))
          << 14;
  }
  MD |= (hasRet ? 1 : 0) << 13;
  MD |= subOpc << 8;

  unsigned resLen = hasRet ? (exSize / getGenxDataportIOSize()) : 0;
  bool forceSplitSend = shouldForceSplitSend(surface);
  if (msgs[1] == 0 && !forceSplitSend) {
    vISA_ASSERT(sizes[1] == 0,
                "Expect the 2nd part of the payload has zero size!");
    createSendInst(pred, dst, msgs[0], sizes[0], resLen, instExSize, MD, sfid,
                   useHeader, SendAccess::READ_WRITE, surface, NULL, instOpt,
                   false);
  } else {
    createSplitSendInst(pred, dst, msgs[0], sizes[0], msgs[1], sizes[1], resLen,
                        instExSize, MD, sfid, useHeader, SendAccess::READ_WRITE,
                        surface, NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

// build the address payload for typed messages (read/write/atomic)
// sources stores the address payload, and its length len is also updated
void IR_Builder::buildTypedSurfaceAddressPayload(
    G4_SrcRegRegion *uOffsetOpnd, G4_SrcRegRegion *vOffsetOpnd,
    G4_SrcRegRegion *rOffsetOpnd, G4_SrcRegRegion *lodOpnd, G4_ExecSize exSize,
    G4_InstOpts instOpt, PayloadSource sources[], uint32_t &len) {
  // Valid address payload pattern are listed below:
  // (* means the parameter is ignored by HW but must be included in payload)
  // U
  // U, V
  // U, V, R
  // U, *, *, LOD
  // U, V, *, LOD
  // U, V, R, LOD

  // Append U
  sources[len].opnd = uOffsetOpnd;
  sources[len].numElts = exSize;
  sources[len].instOpt = instOpt;
  ++len;

  // Append V if any.
  if (!vOffsetOpnd->isNullReg()) {
    sources[len].opnd = vOffsetOpnd;
    sources[len].numElts = exSize;
    sources[len].instOpt = instOpt;
    ++len;
  } else if (!lodOpnd->isNullReg()) {
    G4_SrcRegRegion *nullVOffset = createNullSrc(Type_UD);
    sources[len].opnd = nullVOffset;
    sources[len].numElts = exSize;
    sources[len].instOpt = instOpt;
    ++len;
  }

  // Append R if any.
  if (!rOffsetOpnd->isNullReg()) {
    vISA_ASSERT_INPUT(!vOffsetOpnd->isNullReg(),
                "r offset must be NULL if v offset is NULL");
    sources[len].opnd = rOffsetOpnd;
    sources[len].numElts = exSize;
    sources[len].instOpt = instOpt;
    ++len;
  } else if (!lodOpnd->isNullReg()) {
    G4_SrcRegRegion *nullROffset = createNullSrc(Type_UD);
    sources[len].opnd = nullROffset;
    sources[len].numElts = exSize;
    sources[len].instOpt = instOpt;
    ++len;
  }

  // Append LOD if any.
  if (!lodOpnd->isNullReg()) {
    sources[len].opnd = lodOpnd;
    sources[len].numElts = exSize;
    sources[len].instOpt = instOpt;
    ++len;
  }
}

// u must not be V0. v and r are allowed to be V0, in which case they will be
// skipped in payload.
int IR_Builder::translateVISAGather4TypedInst(
    G4_Predicate *pred, VISA_EMask_Ctrl emask, ChannelMask chMask,
    G4_Operand *surface, VISA_Exec_Size executionSize,
    G4_SrcRegRegion *uOffsetOpnd, G4_SrcRegRegion *vOffsetOpnd,
    G4_SrcRegRegion *rOffsetOpnd, G4_SrcRegRegion *lodOpnd,
    G4_DstRegRegion *dstOpnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize exSize = executionSize == EXEC_SIZE_16 ? g4::SIMD16 : g4::SIMD8;
  vISA_ASSERT_INPUT((exSize == 8 || hasSIMD16TypedRW()), "only simd8 is supported");
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, exSize);
  int numEnabledChannels = chMask.getNumEnabledChannels();

  bool useSplitSend = useSends();

  bool hasHeader = getPlatform() == GENX_BDW;

  PayloadSource sources[5]; // (maybe header) + maximal 4 addresses
  unsigned len = 0;

  if (hasHeader) {
    // Build header
    G4_Declare *dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);
    BuildMH1_BTS_PSM(this, dcl);

    // Append header
    G4_SrcRegRegion *header = createSrcRegRegion(dcl, getRegionStride1());
    sources[len].opnd = header;
    sources[len].numElts = numEltPerGRF<Type_UD>();
    sources[len].instOpt = InstOpt_WriteEnable;
    sources[len].copyExecSize = g4::SIMD8;
    ++len;
  }

  buildTypedSurfaceAddressPayload(uOffsetOpnd, vOffsetOpnd, rOffsetOpnd,
                                  lodOpnd, exSize, instOpt, sources, len);
  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, exSize, useSplitSend, sources, len);

  // bit 8-11: RGBA channel enable
  unsigned msgDesc = chMask.getHWEncoding() << 8;
  SFID sfId;

  // DC1
  // bit14-17: 0101 (read), 1101 (write)
  msgDesc |= DC1_TYPED_SURFACE_READ << 14;
  // bit12-13: 01 (use low 8 slot)
  msgDesc |= MDC_SG3_SG8L << 12;
  sfId = SFID::DP_DC1;

  bool forceSplitSend = shouldForceSplitSend(surface);
  if (msgs[1] == 0 && !forceSplitSend) {
    vISA_ASSERT(sizes[1] == 0,
                "Expect the 2nd part of the payload has zero size!");
    createSendInst(pred, dstOpnd, msgs[0], sizes[0], numEnabledChannels, exSize,
                   msgDesc, sfId, hasHeader, SendAccess::READ_ONLY, surface,
                   nullptr, instOpt, false);
  } else {
    createSplitSendInst(pred, dstOpnd, msgs[0], sizes[0], msgs[1], sizes[1],
                        numEnabledChannels, exSize, msgDesc, sfId, hasHeader,
                        SendAccess::READ_ONLY, surface, nullptr, instOpt,
                        false);
  }

  return VISA_SUCCESS;
}

// u must not be V0. v and r are allowed to be V0, in which case they will be
// skipped in payload.
int IR_Builder::translateVISAScatter4TypedInst(
    G4_Predicate *pred, VISA_EMask_Ctrl emask, ChannelMask chMask,
    G4_Operand *surface, VISA_Exec_Size executionSize,
    G4_SrcRegRegion *uOffsetOpnd, G4_SrcRegRegion *vOffsetOpnd,
    G4_SrcRegRegion *rOffsetOpnd, G4_SrcRegRegion *lodOpnd,
    G4_SrcRegRegion *srcOpnd) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  G4_ExecSize exSize = executionSize == EXEC_SIZE_16 ? g4::SIMD16 : g4::SIMD8;
  vISA_ASSERT_INPUT((exSize == g4::SIMD8 || hasSIMD16TypedRW()),
         "only simd8 is supported");
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, exSize);
  int numEnabledChannels = chMask.getNumEnabledChannels();

  bool useSplitSend = useSends();

  bool hasHeader = getPlatform() == GENX_BDW;

  PayloadSource sources[6]; // (maybe header) + maximal 4 addresses + source
  unsigned len = 0;

  if (hasHeader) {
    // Build header
    G4_Declare *dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);
    BuildMH1_BTS_PSM(this, dcl);

    // Append header
    G4_SrcRegRegion *header = createSrcRegRegion(dcl, getRegionStride1());
    sources[len].opnd = header;
    sources[len].numElts = numEltPerGRF<Type_UD>();
    sources[len].instOpt = InstOpt_WriteEnable;
    sources[len].copyExecSize = g4::SIMD8;
    ++len;
  }

  buildTypedSurfaceAddressPayload(uOffsetOpnd, vOffsetOpnd, rOffsetOpnd,
                                  lodOpnd, exSize, instOpt, sources, len);

  // Append source
  sources[len].opnd = srcOpnd;
  sources[len].numElts = exSize * numEnabledChannels;
  sources[len].instOpt = instOpt;
  ++len;

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, exSize, useSplitSend, sources, len);

  // bit 8-11: RGBA channel enable
  unsigned msgDesc = 0;
  SFID sfId;

  // DC1
  // bit14-17: 0101 (read), 1101 (write)
  msgDesc |= DC1_TYPED_SURFACE_WRITE << 14;
  // bit12-13: 01 (use low 8 slot)
  msgDesc |= MDC_SG3_SG8L << 12;
  sfId = SFID::DP_DC1;

  msgDesc |= chMask.getHWEncoding() << 8;

  G4_DstRegRegion *dstOpnd = createNullDst(Type_UD);

  bool forceSplitSend = shouldForceSplitSend(surface);
  if (msgs[1] == 0 && !forceSplitSend) {
    vISA_ASSERT(sizes[1] == 0,
                "Expect the 2nd part of the payload has zero size!");
    createSendInst(pred, dstOpnd, msgs[0], sizes[0], 0, exSize, msgDesc, sfId,
                   hasHeader, SendAccess::WRITE_ONLY, surface, NULL, instOpt,
                   false);
  } else {
    createSplitSendInst(pred, dstOpnd, msgs[0], sizes[0], msgs[1], sizes[1], 0,
                        exSize, msgDesc, sfId, hasHeader,
                        SendAccess::WRITE_ONLY, surface, NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISATypedAtomicInst(
    VISAAtomicOps atomicOp, bool is16Bit, G4_Predicate *pred,
    VISA_EMask_Ctrl emask, VISA_Exec_Size execSize, G4_Operand *surface,
    G4_SrcRegRegion *uOffsetOpnd, G4_SrcRegRegion *vOffsetOpnd,
    G4_SrcRegRegion *rOffsetOpnd, G4_SrcRegRegion *lodOpnd,
    G4_SrcRegRegion *src0, G4_SrcRegRegion *src1, G4_DstRegRegion *dst) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  VISA_Exec_Size instExecSize = execSize;
  vISA_ASSERT_INPUT(execSize <=
             (getNativeExecSize() == g4::SIMD8 ? EXEC_SIZE_8 : EXEC_SIZE_16),
         "send exec size must not exceed the platform's native execution size");

  unsigned op = Get_Atomic_Op(atomicOp);

  G4_ExecSize exSize{Get_VISA_Exec_Size(execSize)};
  G4_ExecSize instExSize{Get_VISA_Exec_Size(instExecSize)};
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, instExSize);

  if (atomicOp == ATOMIC_CMPXCHG) {
    // we have to swap src0 and src1 since vISA has them in different order from
    // HW
    G4_SrcRegRegion *tmp = src0;
    src0 = src1;
    src1 = tmp;
  }

  bool useSplitSend = useSends();

  PayloadSource sources[6]; // u, v, r, lod, src0, src1
  unsigned len = 0;

  buildTypedSurfaceAddressPayload(uOffsetOpnd, vOffsetOpnd, rOffsetOpnd,
                                  lodOpnd, exSize, instOpt, sources, len);

  if (src0 != nullptr && !src0->isNullReg()) {
    sources[len].opnd = src0;
    sources[len].numElts = exSize;
    sources[len].instOpt = instOpt;
    ++len;
  }

  if (src1 != nullptr && !src1->isNullReg()) {
    sources[len].opnd = src1;
    sources[len].numElts = exSize;
    sources[len].instOpt = instOpt;
    ++len;
  }

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, std::min(exSize, instExSize), useSplitSend,
                 sources, len);

  unsigned dstLength = dst->isNullReg() ? 0 : 1;

  unsigned msgDesc = 0;
  // BTI is filled later
  msgDesc |= op << 8;
  msgDesc |= (dstLength != 0 ? 1 : 0) << 13;

  if (is16Bit) {
    msgDesc |= DC1_TYPED_HALF_INTEGER_ATOMIC << 14;
  } else {
    msgDesc |= DC1_TYPED_ATOMIC << 14;
  }

  bool forceSplitSend = shouldForceSplitSend(surface);
  if (msgs[1] == 0 && !forceSplitSend) {
    vISA_ASSERT(sizes[1] == 0,
                "Expect the 2nd part of the payload has zero size!");
    createSendInst(pred, dst, msgs[0], sizes[0], dstLength, exSize, msgDesc,
                   SFID::DP_DC1, false, SendAccess::READ_WRITE, surface,
                   nullptr, instOpt, false);
  } else {
    createSplitSendInst(pred, dst, msgs[0], sizes[0], msgs[1], sizes[1],
                        dstLength, exSize, msgDesc, SFID::DP_DC1, false,
                        SendAccess::READ_WRITE, surface, nullptr, instOpt,
                        false);
  }

  return VISA_SUCCESS;
}

// apply the sideband offset (can be either imm or variable) to the message
// descriptor
void IR_Builder::applySideBandOffset(G4_Operand *sideBand,
                                     const G4_SendDescRaw *sendMsgDesc) {
#define SIDEBAND_OFFSET_IN_EXDESC 12

  if (sideBand->isImm()) {
    // mov (1) a0.0 sideband << 0xC
    uint32_t sidebandInDesc =
        (uint32_t)(sideBand->asImm()->getImm() << SIDEBAND_OFFSET_IN_EXDESC);
    G4_DstRegRegion *dst = createDstRegRegion(builtinA0, 1);
    createMov(g4::SIMD1, dst, createImm(sidebandInDesc, Type_UD),
              InstOpt_WriteEnable, true);
  } else {
    vISA_ASSERT(sideBand->isSrcRegRegion(),
                 "sideband offset should be a srcRegRegion");
    // shl (1) a0.0 sideband 0xC
    G4_DstRegRegion *dst = createDstRegRegion(builtinA0, 1);
    createBinOp(G4_shl, g4::SIMD1, dst, sideBand,
                createImm(SIDEBAND_OFFSET_IN_EXDESC, Type_UW),
                InstOpt_WriteEnable, true);
  }

  // add (1) a0.0 a0.0 MD
  G4_DstRegRegion *a0Dst = createDstRegRegion(builtinA0, 1);
  G4_SrcRegRegion *a0Src = createSrcRegRegion(builtinA0, getRegionScalar());
  createBinOp(G4_add, g4::SIMD1, a0Dst, a0Src,
              createImm(sendMsgDesc->getExtendedDesc(), Type_UD),
              InstOpt_WriteEnable, true);
}

int IR_Builder::translateVISAGather4ScaledInst(
    G4_Predicate *pred, VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
    ChannelMask chMask, G4_Operand *surface, G4_Operand *globalOffset,
    G4_SrcRegRegion *offsets, G4_DstRegRegion *dst) {
  return translateGather4Inst(pred, execSize, eMask, chMask, surface,
                              globalOffset, offsets, dst);
}

int IR_Builder::translateVISAScatter4ScaledInst(
    G4_Predicate *pred, VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
    ChannelMask chMask, G4_Operand *surface, G4_Operand *globalOffset,
    G4_SrcRegRegion *offsets, G4_SrcRegRegion *src) {
  return translateScatter4Inst(pred, execSize, eMask, chMask, surface,
                               globalOffset, offsets, src);
}

int IR_Builder::translateGather4Inst(
    G4_Predicate *pred, VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
    ChannelMask chMask, G4_Operand *surface, G4_Operand *globalOffset,
    G4_SrcRegRegion *offsets, G4_DstRegRegion *dst) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  vISA_ASSERT_INPUT(execSize == EXEC_SIZE_1 || execSize == EXEC_SIZE_2 ||
                  execSize == EXEC_SIZE_4 || execSize == EXEC_SIZE_8 ||
                  execSize == EXEC_SIZE_16,
              "Only support SIMD1, SIMD2, SIMD4, SIMD8 or SIMD16!");

  VISA_Exec_Size instExecSize = execSize;
  execSize = roundUpExecSize(execSize);

  G4_ExecSize exSize = toExecSize(execSize);
  G4_ExecSize instExSize = toExecSize(instExecSize);
  unsigned instOpt = Get_Gen4_Emask(eMask, exSize);

  bool useSplitSend = useSends();
  bool useHeader =
      needsA32MsgHeader() && surface && isStatelessSurface(surface);

  // In case non-zero global offset is specified, we need to recalculate
  // offsets.
  if (!globalOffset->isImm() || globalOffset->asImm()->getImm() != 0) {
    G4_Declare *dcl = createSendPayloadDcl(exSize, offsets->getType());
    G4_DstRegRegion *tmp = createDstRegRegion(dcl, 1);
    G4_Predicate *addPred = duplicateOperand(pred);
    createInst(addPred, G4_add, 0, g4::NOSAT, instExSize, tmp, offsets,
               globalOffset, instOpt, true);
    offsets = createSrcRegRegion(dcl, getRegionStride1());
  }

  PayloadSource sources[2]; // Maximal 2 sources, optional header + offsets
  unsigned len = 0;

  if (useHeader) {
    G4_Declare *dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);

    BuildMH1_A32_PSM(this, dcl);

    G4_SrcRegRegion *header = createSrcRegRegion(dcl, getRegionStride1());
    sources[len].opnd = header;
    sources[len].numElts = numEltPerGRF<Type_UD>();
    sources[len].instOpt = InstOpt_WriteEnable;
    sources[len].copyExecSize = g4::SIMD8;
    ++len;
  }

  sources[len].opnd = offsets;
  sources[len].numElts = exSize;
  sources[len].instOpt = instOpt;
  ++len;

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, std::min(exSize, instExSize), useSplitSend,
                 sources, len);

  SFID sfid = SFID::DP_DC1;

  unsigned MD = 0;
  // Leave sidebind scale offset 0 as it is not used now.
  MD |= DC1_UNTYPED_SURFACE_READ << 14;
  MD |= (execSize == EXEC_SIZE_8 ? MDC_SM3_SIMD8 : MDC_SM3_SIMD16) << 12;
  MD |= chMask.getHWEncoding() << 8;

  unsigned resLen =
      (exSize / getGenxDataportIOSize()) * chMask.getNumEnabledChannels();

  bool forceSplitSend = shouldForceSplitSend(surface);
  if (msgs[1] == 0 && !forceSplitSend) {
    vISA_ASSERT(sizes[1] == 0,
                "Expect the 2nd part of the payload has zero size!");
    createSendInst(pred, dst, msgs[0], sizes[0], resLen, instExSize, MD, sfid,
                   useHeader, SendAccess::READ_ONLY, surface, NULL, instOpt,
                   false);
  } else {
    createSplitSendInst(pred, dst, msgs[0], sizes[0], msgs[1], sizes[1], resLen,
                        instExSize, MD, sfid, useHeader, SendAccess::READ_ONLY,
                        surface, NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateScatter4Inst(
    G4_Predicate *pred, VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
    ChannelMask chMask, G4_Operand *surface, G4_Operand *globalOffset,
    G4_SrcRegRegion *offsets, G4_SrcRegRegion *src) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  vISA_ASSERT_INPUT(execSize == EXEC_SIZE_1 || execSize == EXEC_SIZE_2 ||
                  execSize == EXEC_SIZE_4 || execSize == EXEC_SIZE_8 ||
                  execSize == EXEC_SIZE_16,
              "Only support SIMD1, SIMD2, SIMD4, SIMD8 or SIMD16!");

  VISA_Exec_Size instExecSize = execSize;
  execSize = roundUpExecSize(execSize);

  G4_ExecSize exSize = toExecSize(execSize);
  G4_ExecSize instExSize = toExecSize(instExecSize);
  unsigned instOpt = Get_Gen4_Emask(eMask, exSize);

  bool useSplitSend = useSends();
  bool useHeader =
      needsA32MsgHeader() && surface && isStatelessSurface(surface);

  // In case non-zero global offset is specified, we need to recalculate
  // offsets.
  if (!globalOffset->isImm() || globalOffset->asImm()->getImm() != 0) {
    G4_Declare *dcl = createSendPayloadDcl(exSize, offsets->getType());
    G4_DstRegRegion *tmp = createDstRegRegion(dcl, 1);
    G4_Predicate *addPred = duplicateOperand(pred);
    createInst(addPred, G4_add, 0, g4::NOSAT, instExSize, tmp, offsets,
               globalOffset, instOpt, true);
    offsets = createSrcRegRegion(dcl, getRegionStride1());
  }

  PayloadSource
      sources[3]; // Maximal 3 sources, optional header + offsets + src
  unsigned len = 0;

  if (useHeader) {
    G4_Declare *dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);

    // TODO: Get PSM supported on demand.
    BuildMH1_A32_PSM(this, dcl);

    G4_SrcRegRegion *header = createSrcRegRegion(dcl, getRegionStride1());
    sources[len].opnd = header;
    sources[len].numElts = numEltPerGRF<Type_UD>();
    sources[len].instOpt = InstOpt_WriteEnable;
    sources[len].copyExecSize = g4::SIMD8;
    ++len;
  }

  sources[len].opnd = offsets;
  sources[len].numElts = exSize;
  sources[len].instOpt = instOpt;
  ++len;
  sources[len].opnd = src;
  sources[len].numElts = exSize * chMask.getNumEnabledChannels();
  sources[len].instOpt = instOpt;
  ++len;

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, std::min(exSize, instExSize), useSplitSend,
                 sources, len);

  SFID sfid = SFID::DP_DC1;

  unsigned MD = 0;
  // Leave sidebind scale offset 0 as it is not used now.
  MD |= DC1_UNTYPED_SURFACE_WRITE << 14;
  MD |= (execSize == EXEC_SIZE_8 ? MDC_SM3_SIMD8 : MDC_SM3_SIMD16) << 12;
  MD |= chMask.getHWEncoding() << 8;

  G4_DstRegRegion *dst = createNullDst(Type_UD);
  bool forceSplitSend = shouldForceSplitSend(surface);
  if (msgs[1] == 0 && !forceSplitSend) {
    vISA_ASSERT(sizes[1] == 0,
                "Expect the 2nd part of the payload has zero size!");
    createSendInst(pred, dst, msgs[0], sizes[0], 0, instExSize, MD, sfid,
                   useHeader, SendAccess::WRITE_ONLY, surface, NULL, instOpt,
                   false);
  } else {
    createSplitSendInst(pred, dst, msgs[0], sizes[0], msgs[1], sizes[1], 0,
                        instExSize, MD, sfid, useHeader, SendAccess::WRITE_ONLY,
                        surface, NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

/// GetNumBatch() - return the number of batches required to copy the raw
/// operand to message payload
static unsigned GetNumBatch(VISA_SVM_Block_Type blockSize,
                            VISA_SVM_Block_Num numBlocks) {
  switch (blockSize) {
  case SVM_BLOCK_TYPE_BYTE:
    switch (numBlocks) {
    case SVM_BLOCK_NUM_1:
    case SVM_BLOCK_NUM_2:
    case SVM_BLOCK_NUM_4:
      return 1;
    case SVM_BLOCK_NUM_8:
      return 2;
    }
    break;
  case SVM_BLOCK_TYPE_DWORD:
    return Get_Common_ISA_SVM_Block_Num(numBlocks);
  case SVM_BLOCK_TYPE_QWORD:
    return Get_Common_ISA_SVM_Block_Num(numBlocks);
  }
  vISA_ASSERT_UNREACHABLE("Unhandled sizes/numbers of block/element!");
  return 0;
}

int IR_Builder::translateVISAGatherScaledInst(
    G4_Predicate *pred, VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
    VISA_SVM_Block_Num numBlocks, G4_Operand *surface, G4_Operand *globalOffset,
    G4_SrcRegRegion *offsets, G4_DstRegRegion *dst) {
  return translateByteGatherInst(pred, execSize, eMask, numBlocks, surface,
                                 globalOffset, offsets, dst);
}

int IR_Builder::translateVISAScatterScaledInst(
    G4_Predicate *pred, VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
    VISA_SVM_Block_Num numBlocks, G4_Operand *surface, G4_Operand *globalOffset,
    G4_SrcRegRegion *offsets, G4_SrcRegRegion *src) {
  return translateByteScatterInst(pred, execSize, eMask, numBlocks, surface,
                                  globalOffset, offsets, src);
}

static void BuildMH_A32_GO(IR_Builder *IRB, G4_Declare *header,
                           G4_Operand *globalOffset = 0) {
  // Clear header
  G4_DstRegRegion *h = IRB->createDst(header->getRegVar(), 0, 0, 1, Type_UD);
  IRB->createMov(g4::SIMD8, h, IRB->createImm(0, Type_UD), InstOpt_WriteEnable,
                 true);
  // Copy global offset if necessary.
  if (globalOffset &&
      !(globalOffset->isImm() && globalOffset->asImm()->isZero())) {
    G4_DstRegRegion *gOffDst =
        IRB->createDst(header->getRegVar(), 0, 2, 1, Type_UD);
    IRB->createMov(g4::SIMD1, gOffDst, globalOffset, InstOpt_WriteEnable, true);
  }
}

int IR_Builder::translateByteGatherInst(
    G4_Predicate *pred, VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
    VISA_SVM_Block_Num numBlocks, G4_Operand *surface, G4_Operand *globalOffset,
    G4_SrcRegRegion *offsets, G4_DstRegRegion *dst) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  vISA_ASSERT_INPUT(execSize == EXEC_SIZE_1 || execSize == EXEC_SIZE_2 ||
                  execSize == EXEC_SIZE_4 || execSize == EXEC_SIZE_8 ||
                  execSize == EXEC_SIZE_16,
              "Only support SIMD1, SIMD2, SIMD4, SIMD8 or SIMD16!");
  vISA_ASSERT_INPUT(numBlocks == SVM_BLOCK_NUM_1 || numBlocks == SVM_BLOCK_NUM_2 ||
                  numBlocks == SVM_BLOCK_NUM_4,
              "Byte gather ONLY supports 1, 2, and 4 elements per slot!");

  VISA_Exec_Size instExecSize = execSize;
  execSize = roundUpExecSize(execSize);

  G4_ExecSize exSize{Get_VISA_Exec_Size(execSize)};
  G4_ExecSize instExSize{Get_VISA_Exec_Size(instExecSize)};
  G4_InstOpts instOpt = Get_Gen4_Emask(eMask, instExSize);
  unsigned numBatch = GetNumBatch(SVM_BLOCK_TYPE_BYTE, numBlocks);

  bool isSLM = IsSLMSurface(surface);
  // SLM forbids header. Header is optional in A32 when both scale and global
  // offset are 0s.
  bool useHeader = !isSLM && needsA32MsgHeader();
  bool useSplitSend = useSends();

  // In case non-zero global offset is specified, we need to recalculate
  // offsets.
  //
  // NOTE: Even though pre-SKL devices require header, eliminating global
  //       offset by adjusting offsets will simplify the header generation.
  if (!globalOffset->isImm() || globalOffset->asImm()->getImm() != 0) {
    G4_Declare *dcl = createSendPayloadDcl(exSize, offsets->getType());
    G4_DstRegRegion *tmp = createDstRegRegion(dcl, 1);
    createBinOp(G4_add, instExSize, tmp, offsets, globalOffset, instOpt, true);
    offsets = createSrcRegRegion(dcl, getRegionStride1());
  }

  PayloadSource sources[2]; // Maximal 2 sources, optional header + offsets
  unsigned len = 0;

  if (useHeader) {
    G4_Declare *dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);

    // TODO: Get BTS supported on demand.
    BuildMH_A32_GO(this, dcl);

    G4_SrcRegRegion *header = createSrcRegRegion(dcl, getRegionStride1());
    sources[len].opnd = header;
    sources[len].numElts = numEltPerGRF<Type_UD>();
    sources[len].instOpt = InstOpt_WriteEnable;
    sources[len].copyExecSize = g4::SIMD8;
    ++len;
  }

  sources[len].opnd = offsets;
  sources[len].numElts = exSize;
  sources[len].instOpt = instOpt;
  ++len;

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, std::min(exSize, instExSize), useSplitSend,
                 sources, len);

  SFID sfid = SFID::DP_DC0;

  unsigned MD = 0;
  MD |= DC_BYTE_SCATTERED_READ << 14;
  MD |= numBlocks << 10;
  MD |= (execSize == EXEC_SIZE_8 ? MDC_SM2_SIMD8 : MDC_SM2_SIMD16) << 8;

  unsigned resLen = (exSize / getGenxDataportIOSize()) * numBatch;
  bool forceSplitSend = shouldForceSplitSend(surface);
  if (msgs[1] == 0 && !forceSplitSend) {
    vISA_ASSERT(sizes[1] == 0,
                "Expect the 2nd part of the payload has zero size!");
    createSendInst(pred, dst, msgs[0], sizes[0], resLen, instExSize, MD, sfid,
                   useHeader, SendAccess::READ_ONLY, surface, NULL, instOpt,
                   false);
  } else {
    createSplitSendInst(pred, dst, msgs[0], sizes[0], msgs[1], sizes[1], resLen,
                        instExSize, MD, sfid, useHeader, SendAccess::READ_ONLY,
                        surface, NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateByteScatterInst(
    G4_Predicate *pred, VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
    VISA_SVM_Block_Num numBlocks, G4_Operand *surface, G4_Operand *globalOffset,
    G4_SrcRegRegion *offsets, G4_SrcRegRegion *src) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  vISA_ASSERT_INPUT(execSize == EXEC_SIZE_1 || execSize == EXEC_SIZE_2 ||
                  execSize == EXEC_SIZE_4 || execSize == EXEC_SIZE_8 ||
                  execSize == EXEC_SIZE_16,
              "Only support SIMD1, SIMD2, SIMD4, SIMD8 or SIMD16!");
  vISA_ASSERT_INPUT(numBlocks == SVM_BLOCK_NUM_1 || numBlocks == SVM_BLOCK_NUM_2 ||
                  numBlocks == SVM_BLOCK_NUM_4,
              "Byte scatter ONLY supports 1, 2, and 4 elements per slot!");

  VISA_Exec_Size instExecSize = execSize;
  execSize = roundUpExecSize(execSize);

  G4_ExecSize exSize{Get_VISA_Exec_Size(execSize)};
  G4_ExecSize instExSize{Get_VISA_Exec_Size(instExecSize)};
  G4_InstOpts instOpt = Get_Gen4_Emask(eMask, exSize);
  unsigned numBatch = GetNumBatch(SVM_BLOCK_TYPE_BYTE, numBlocks);

  bool isSLM = IsSLMSurface(surface);
  // SLM forbids header. Header is optional in A32 when both scale and global
  // offset are 0s.
  bool useHeader = !isSLM && needsA32MsgHeader();
  bool useSplitSend = useSends();

  // In case non-zero global offset is specified, we need to recalculate
  // offsets.
  //
  // NOTE: Even though pre-SKL devices require header, eliminating global
  //       offset by adjusting offsets will simplify the header generation.
  if (!globalOffset->isImm() || globalOffset->asImm()->getImm() != 0) {
    G4_Declare *dcl = createSendPayloadDcl(exSize, offsets->getType());
    G4_DstRegRegion *tmp = createDstRegRegion(dcl, 1);
    createBinOp(G4_add, instExSize, tmp, offsets, globalOffset, instOpt, true);
    offsets = createSrcRegRegion(dcl, getRegionStride1());
  }

  PayloadSource
      sources[3]; // Maximal 2 sources, optional header + offsets + src
  unsigned len = 0;

  if (useHeader) {
    G4_Declare *dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);

    // TODO: Get BTS supported on demand.
    BuildMH_A32_GO(this, dcl);

    G4_SrcRegRegion *header = createSrcRegRegion(dcl, getRegionStride1());
    sources[len].opnd = header;
    sources[len].numElts = numEltPerGRF<Type_UD>();
    sources[len].instOpt = InstOpt_WriteEnable;
    sources[len].copyExecSize = g4::SIMD8;
    ++len;
  }

  sources[len].opnd = offsets;
  sources[len].numElts = exSize;
  sources[len].instOpt = instOpt;
  ++len;
  sources[len].opnd = src;
  sources[len].numElts = exSize * numBatch;
  sources[len].instOpt = instOpt;
  ++len;

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, std::min(exSize, instExSize), useSplitSend,
                 sources, len);

  SFID sfid = SFID::DP_DC0;

  unsigned MD = 0;
  // Leave sidebind scale offset 0 as it is not used now.
  MD |= DC_BYTE_SCATTERED_WRITE << 14;
  MD |= numBlocks << 10;
  MD |= (execSize == EXEC_SIZE_8 ? MDC_SM2_SIMD8 : MDC_SM2_SIMD16) << 8;

  G4_DstRegRegion *dst = createNullDst(Type_UD);
  bool forceSplitSend = shouldForceSplitSend(surface);
  if (msgs[1] == 0 && !forceSplitSend) {
    vISA_ASSERT(sizes[1] == 0,
                "Expect the 2nd part of the payload has zero size!");
    createSendInst(pred, dst, msgs[0], sizes[0], 0, instExSize, MD, sfid,
                   useHeader, SendAccess::WRITE_ONLY, surface, NULL, instOpt,
                   false);
  } else {
    createSplitSendInst(pred, dst, msgs[0], sizes[0], msgs[1], sizes[1], 0,
                        instExSize, MD, sfid, useHeader, SendAccess::WRITE_ONLY,
                        surface, NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

#define SEND_GT_MSG_TYPE_BIT 14

///
/// Descriptors for OWord Block read/write
/// Bits 31-29: Reserved
/// Bits 28-25: Message Length: Total 256bit registers expected to be sent.
/// Bits 24-20: Response Length: Total 256bit registers expected in response.
/// Bit  19:    Does this Message Descriptor have a header? 1 Yes, 0 No.
/// Bits 18-14: Message Type: 10100: A64 Block Read, 10101: A64 Block Write
/// Bit  13:    Ignore
/// Bits 12-11: Message sub-type (00 for OWord Block Read/Write, 01 for
/// Unaligned OWord Block Read/Write)
/// Bits 10-8:  Block Size, 000 for 1 OWord,
/// 001 for 2 OWords, 010 for 4 OWords, 100 for 8 OWords.
/// Bits 7-0:   Binding
/// Table Index: Set to 0xFF for stateless memory space used bu A64 SVM Data
/// Port.
int IR_Builder::translateVISASVMBlockReadInst(VISA_Oword_Num size,
                                              bool unaligned,
                                              G4_Operand *address,
                                              G4_DstRegRegion *dst) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  unsigned numOword = Get_VISA_Oword_Num(size);

  G4_Declare *dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);
  G4_Declare *dclAsUQ =
      createSendPayloadDcl(getGenxDataportIOSize() / 2, Type_UQ);
  dclAsUQ->setAliasDeclare(dcl, 0);
  if (noInt64()) {
    G4_SrcRegRegion *region = address->asSrcRegRegion();
    G4_SrcRegRegion *tmp;
    tmp = createSrcRegRegion(Mod_src_undef, region->getRegAccess(),
                             region->getBase(), region->getRegOff(),
                             region->getSubRegOff() * 2, region->getRegion(),
                             Type_UD);
    createMovInst(dcl, 0, 0, g4::SIMD1, NULL, NULL, tmp, true);
    tmp = createSrcRegRegion(Mod_src_undef, region->getRegAccess(),
                             region->getBase(), region->getRegOff(),
                             region->getSubRegOff() * 2 + 1,
                             region->getRegion(), Type_UD);
    createMovInst(dcl, 0, 1, g4::SIMD1, NULL, NULL, tmp, true);
  } else {
    createMovInst(dclAsUQ, 0, 0, g4::SIMD1, NULL, NULL, address, true);
  }


  // This is a WA for fused EU where
  // execution size greater than SIMD size will cause problems
  // 14019074860: the if-condition needs to be replaced when WA Id is available
  if (getPlatform() == Xe_DG2) {
    LSC_DATA_SHAPE dataShape{};
    dataShape.order = LSC_DATA_ORDER_TRANSPOSE;
    dataShape.size = LSC_DATA_SIZE_32b;
    dataShape.elems = lscGetElementNum(numOword * 16 / 4);

    G4_SrcRegRegion *srcAddr64 = createSrcRegRegion(dclAsUQ, getRegionStride1());
    return translateLscUntypedInst(
        LSC_LOAD, LSC_UGM, nullptr,
        EXEC_SIZE_1,
        vISA_EMASK_M1_NM,
        {LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT},
        {LSC_ADDR_TYPE_FLAT, 1, 0, LSC_ADDR_SIZE_64b},
        dataShape,
        nullptr, 0x0, // surface, ssidx
        dst->asDstRegRegion(), srcAddr64,
        nullptr,
        nullptr,
        nullptr);
  }
  DATA_CACHE1_MESSAGES msgSubOpcode = DC1_A64_BLOCK_READ;
  unsigned rspLength = ((numOword * 16 - 1) / getGRFSize() + 1);

  unsigned desc =
      getA64BTI() |
      (unaligned ? A64_BLOCK_MSG_OWORD_UNALIGNED_READ : A64_BLOCK_MSG_OWORD_RW)
          << A64_BLOCK_MSG_SUBTYPE_OFFSET |
      msgSubOpcode << SEND_GT_MSG_TYPE_BIT;

  desc = setOwordForDesc(desc, numOword);

  G4_ExecSize sendExecSize{FIX_OWORD_SEND_EXEC_SIZE(numOword)};
  dst->setType(*this, Type_UD);

  G4_SrcRegRegion *srcAddr = createSrcRegRegion(dcl, getRegionStride1());
  createSendInst(NULL, dst, srcAddr, 1, rspLength, sendExecSize, desc,
                 SFID::DP_DC1, true, SendAccess::READ_ONLY, NULL, NULL,
                 InstOpt_WriteEnable, false);

  return VISA_SUCCESS;
}

int IR_Builder::translateVISASVMBlockWriteInst(VISA_Oword_Num size,
                                               G4_Operand *address,
                                               G4_SrcRegRegion *src) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  unsigned numOword = Get_VISA_Oword_Num(size);
  unsigned srcNumGRF = (numOword * 16 + getGRFSize() - 1) / getGRFSize();
  G4_ExecSize sendExecSize{FIX_OWORD_SEND_EXEC_SIZE(numOword)};

  // FIXME: may want to apply this to FIX_OWORD_SEND_EXEC_SIZE instead
  if (sendExecSize < g4::SIMD8) {
    sendExecSize = g4::SIMD8;
  }

  G4_Declare *dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);
  if (noInt64()) {
    G4_SrcRegRegion *region = address->asSrcRegRegion();
    G4_SrcRegRegion *tmp;
    tmp = createSrcRegRegion(Mod_src_undef, region->getRegAccess(),
                             region->getBase(), region->getRegOff(),
                             region->getSubRegOff() * 2, region->getRegion(),
                             Type_UD);
    createMovInst(dcl, 0, 0, g4::SIMD1, NULL, NULL, tmp, true);
    tmp = createSrcRegRegion(Mod_src_undef, region->getRegAccess(),
                             region->getBase(), region->getRegOff(),
                             region->getSubRegOff() * 2 + 1,
                             region->getRegion(), Type_UD);
    createMovInst(dcl, 0, 1, g4::SIMD1, NULL, NULL, tmp, true);
  } else {
    G4_Declare *dclAsUQ =
        createSendPayloadDcl(getGenxDataportIOSize() / 2, Type_UQ);
    dclAsUQ->setAliasDeclare(dcl, 0);
    createMovInst(dclAsUQ, 0, 0, g4::SIMD1, NULL, NULL, address, true);
  }

  bool useSplitSend = useSends();
  PayloadSource sources[2];
  unsigned len = 0;

  sources[len].opnd = createSrcRegRegion(dcl, getRegionStride1());
  sources[len].numElts = numEltPerGRF<Type_UD>();
  sources[len].instOpt = InstOpt_WriteEnable;
  sources[len].copyExecSize = g4::SIMD8; // block msg header has 8 DWs
  ++len;

  if (src->getElemSize() < TypeSize(Type_UD)) {
    // use D for size computation. Src is guaranteed to be GRF-aligend per vISA
    // spec
    src->setType(*this, Type_UD);
  }
  sources[len].opnd = src;

  G4_ExecSize movExecSize{0};

  auto scale = getGRFSize() / src->getElemSize();
  switch (src->getElemSize()) {
  case 4:
    sources[len].numElts = scale * srcNumGRF;
    movExecSize = G4_ExecSize(scale);
    break;
  case 8:
    sources[len].numElts = scale * srcNumGRF;
    movExecSize = G4_ExecSize(scale);
    break;
  }

  sources[len].instOpt = InstOpt_WriteEnable;
  ++len;

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, movExecSize, useSplitSend, sources, len);

  DATA_CACHE1_MESSAGES msgSubOpcode = DC1_A64_BLOCK_WRITE;

  unsigned desc = getA64BTI() |
                  A64_BLOCK_MSG_OWORD_RW << A64_BLOCK_MSG_SUBTYPE_OFFSET |
                  msgSubOpcode << SEND_GT_MSG_TYPE_BIT;

  desc = setOwordForDesc(desc, numOword);

  G4_DstRegRegion *sendDst = createNullDst(Type_UD);

  if (msgs[1] == 0) {
    createSendInst(NULL, sendDst, msgs[0], sizes[0], 0, sendExecSize, desc,
                   SFID::DP_DC1, true, SendAccess::WRITE_ONLY, NULL, NULL,
                   InstOpt_WriteEnable, false);
  } else {
    createSplitSendInst(NULL, sendDst, msgs[0], sizes[0], msgs[1], sizes[1], 0,
                        sendExecSize, desc, SFID::DP_DC1, true,
                        SendAccess::WRITE_ONLY, NULL, NULL, InstOpt_WriteEnable,
                        false);
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISASVMScatterReadInst(
    VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask, G4_Predicate *pred,
    VISA_SVM_Block_Type blockSize, VISA_SVM_Block_Num numBlocks,
    G4_SrcRegRegion *addresses, G4_DstRegRegion *dst) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  vISA_ASSERT_INPUT(execSize == EXEC_SIZE_1 || execSize == EXEC_SIZE_2 ||
                  execSize == EXEC_SIZE_4 || execSize == EXEC_SIZE_8 ||
                  execSize == EXEC_SIZE_16,
              "Only support SIMD1, SIMD2, SIMD4, SIMD8 or SIMD16!");

  VISA_Exec_Size instExecSize = execSize;
  execSize = roundUpExecSize(execSize);

  bool is8ByteMsg =
      blockSize == SVM_BLOCK_TYPE_BYTE && numBlocks == SVM_BLOCK_NUM_8;
  vISA_ASSERT_INPUT((!is8ByteMsg || has8ByteA64Gather()),
         "A64 8-byte scatter not supported on this platform");

  G4_ExecSize exSize{Get_VISA_Exec_Size(execSize)};
  G4_ExecSize instExSize{Get_VISA_Exec_Size(instExecSize)};
  G4_InstOpts instOpt = Get_Gen4_Emask(eMask, instExSize);

  uint32_t messageLength = (8 * exSize) / getGRFSize();
  uint32_t numDWperLane = 0;

  // ToDo: remove this as it should be done in HWConformity
  if (instExSize < 8 && WaDisableSendSrcDstOverlap()) {
    // as message length is set to 2 (HW requirements),
    // we have to even align both src/dst to satisfy the WA
    G4_Declare *srcDcl = addresses->getTopDcl()->getRootDeclare();
    if (srcDcl->getByteSize() <= numEltPerGRF<Type_UB>()) {
      srcDcl->setEvenAlign();
    }
    G4_Declare *dstDcl = dst->getTopDcl()->getRootDeclare();
    if (dstDcl->getByteSize() <= numEltPerGRF<Type_UB>()) {
      dstDcl->setEvenAlign();
    }
  }

  switch (blockSize) {
  case SVM_BLOCK_TYPE_BYTE:
    numDWperLane = (numBlocks == SVM_BLOCK_NUM_8) ? 2 : 1;
    break;
  case SVM_BLOCK_TYPE_DWORD:
    numDWperLane = Get_Common_ISA_SVM_Block_Num(numBlocks);
    break;
  case SVM_BLOCK_TYPE_QWORD:
    numDWperLane = Get_Common_ISA_SVM_Block_Num(numBlocks) * 2;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("Illegal SVM block type");
  }
  uint32_t responseLength = (numDWperLane * 4 * exSize) / getGRFSize();

  unsigned desc = 0;
  desc |= getA64BTI();
  desc |= blockSize << 8;
  desc |= numBlocks << 10;
  desc |= (exSize == 8 ? 0 : 1) << 12;
  desc |= DC1_A64_SCATTERED_READ << 14;

  createSendInst(pred, dst, addresses, messageLength, responseLength,
                 instExSize, desc, SFID::DP_DC1, false, SendAccess::READ_ONLY,
                 NULL, NULL, instOpt, false);

  return VISA_SUCCESS;
}

int IR_Builder::translateVISASVMScatterWriteInst(
    VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask, G4_Predicate *pred,
    VISA_SVM_Block_Type blockSize, VISA_SVM_Block_Num numBlocks,
    G4_SrcRegRegion *addresses, G4_SrcRegRegion *src) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  vISA_ASSERT_INPUT(execSize == EXEC_SIZE_1 || execSize == EXEC_SIZE_2 ||
                  execSize == EXEC_SIZE_4 || execSize == EXEC_SIZE_8 ||
                  execSize == EXEC_SIZE_16,
              "Only support SIMD1, SIMD2, SIMD4, SIMD8 or SIMD16!");

  bool is8ByteMsg =
      blockSize == SVM_BLOCK_TYPE_BYTE && numBlocks == SVM_BLOCK_NUM_8;
  vISA_ASSERT_INPUT((!is8ByteMsg || has8ByteA64Gather()),
         "A64 8-byte scatter not supported on this platform");
  VISA_Exec_Size instExecSize = execSize;
  execSize = roundUpExecSize(execSize);

  G4_ExecSize exSize{Get_VISA_Exec_Size(execSize)};
  G4_ExecSize instExSize{Get_VISA_Exec_Size(instExecSize)};
  G4_InstOpts instOpt = Get_Gen4_Emask(eMask, instExSize);

  bool useSplitSend = useSends();

  PayloadSource sources[2]; // Maximal 2 sources, offsets + src
  unsigned len = 0;

  sources[len].opnd = addresses;
  sources[len].numElts = exSize;
  sources[len].instOpt = instOpt;
  ++len;

  unsigned numElems = 1;
  // NOTE that BYTE scatter always has numElems set to 1 as
  // - when the number of data elements is 1, 2, or 4, the writeback payload
  //   is always 1 MDP_DW_SIMD8/_SIMD16.
  // - when the number of data elements is 8, the write payload is always 1
  //   MDP_QW_SIMD8/_SIMD16.
  // This ALSO implies the RAW operand should be in type of UQ when the
  // number of data elements is 8.
  if (blockSize != SVM_BLOCK_TYPE_BYTE)
    numElems = Get_Common_ISA_SVM_Block_Num(numBlocks);

  sources[len].opnd = src;
  sources[len].numElts = exSize * numElems;
  sources[len].instOpt = instOpt;
  ++len;

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};

  // adjust src type
  // PreparePayload takes src type to calculate src1 size. The src type have to
  // be DW for byte scatter read
  G4_Type srcType = src->getType();
  if ((blockSize == SVM_BLOCK_TYPE_BYTE) &&
      (numBlocks == SVM_BLOCK_NUM_1 || numBlocks == SVM_BLOCK_NUM_2) &&
      (TypeSize(srcType) != 4))
    src->setType(*this, Type_UD);

  preparePayload(msgs, sizes, std::min(exSize, instExSize), useSplitSend,
                 sources, len);

  // set the type back in case we changed it for preparePayload
  src->setType(*this, srcType);

  unsigned desc = 0;
  desc |= getA64BTI();
  desc |= blockSize << 8;
  desc |= numBlocks << 10;
  desc |= (exSize == 8 ? 0 : 1) << 12;
  desc |= DC1_A64_SCATTERED_WRITE << 14;

  G4_DstRegRegion *dst = createNullDst(Type_UD);
  if (msgs[1] == 0) {
    vISA_ASSERT(sizes[1] == 0,
                "Expect the 2nd part of the payload has zero size!");
    createSendInst(pred, dst, msgs[0], sizes[0], 0, instExSize, desc,
                   SFID::DP_DC1, false, SendAccess::WRITE_ONLY, NULL, NULL,
                   instOpt, false);
  } else {
    createSplitSendInst(pred, dst, msgs[0], sizes[0], msgs[1], sizes[1], 0,
                        instExSize, desc, SFID::DP_DC1, false,
                        SendAccess::WRITE_ONLY, NULL, NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

// is16Bit indicates if this is a 16bit atomic op. The input source (if
// any) and the writeback (if any) have the same datalayout as dword messages.
// Only the lower 16 bits of each dword is used.
//
static void FillSVMAtomicMsgDesc(bool is16Bit, bool isFloatOp,
                                 uint32_t &msgDesc) {
  if (is16Bit) {
    if (isFloatOp) {
      msgDesc |= DC1_A64_UNTYPED_HALF_FLOAT_ATOMIC << 14;
    } else {
      msgDesc |= DC1_A64_UNTYPED_HALF_INTEGER_ATOMIC << 14;
    }
  } else {
    if (isFloatOp) {
      msgDesc |= DC1_A64_UNTYPED_FLOAT_ATOMIC << 14;
    } else {
      msgDesc |= DC1_A64_ATOMIC << 14;
    }
  }
}

int IR_Builder::translateVISASVMAtomicInst(
    VISAAtomicOps atomicOp, unsigned short bitwidth, VISA_Exec_Size execSize,
    VISA_EMask_Ctrl emask, G4_Predicate *pred, G4_SrcRegRegion *addresses,
    G4_SrcRegRegion *src0, G4_SrcRegRegion *src1, G4_DstRegRegion *dst) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  vISA_ASSERT_INPUT(bitwidth == 16 || bitwidth == 32 || bitwidth == 64,
               "bitwidth must be 16/32/64");

  vISA_ASSERT_INPUT(getPlatform() >= Xe_XeHPSDV ||
                  ((atomicOp != ATOMIC_FADD) && (atomicOp != ATOMIC_FSUB)),
              "FADD/FSUB atomic operations are only supported on this devices");

  VISA_Exec_Size instExecSize = execSize;
  execSize = roundUpExecSize(execSize);

  unsigned op = Get_Atomic_Op(atomicOp);

  G4_ExecSize exSize{Get_VISA_Exec_Size(execSize)};
  G4_ExecSize instExSize{Get_VISA_Exec_Size(instExecSize)};
  G4_InstOpts instOpt = Get_Gen4_Emask(emask, instExSize);

  if (atomicOp == ATOMIC_CMPXCHG) {
    // we have to swap src0 and src1 since vISA has them in different order from
    // HW
    G4_SrcRegRegion *tmp = src0;
    src0 = src1;
    src1 = tmp;
  }

  bool useSplitSend = useSends();

  PayloadSource sources[3]; // addresses, src0, and src1
  unsigned len = 0;

  sources[len].opnd = addresses;
  sources[len].numElts = exSize;
  sources[len].instOpt = instOpt;
  ++len;

  if (src0 != NULL && !src0->isNullReg()) {
    sources[len].opnd = src0;
    sources[len].numElts = exSize;
    sources[len].instOpt = instOpt;
    ++len;
  }

  if (src1 != NULL && !src1->isNullReg()) {
    sources[len].opnd = src1;
    sources[len].numElts = exSize;
    sources[len].instOpt = instOpt;
    ++len;
  }

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  // For send that has smaller execsize than exSize, like
  //     "send (4)  ..."
  // Make sure to use send's execsize (4) as batchsize, not 8/16/32.
  // Thus, batchsize is min(exSize, instExSize).
  preparePayload(msgs, sizes, std::min(exSize, instExSize), useSplitSend,
                 sources, len);
  unsigned dstLength =
      dst->isNullReg() ? 0 : ((bitwidth == 16 || bitwidth == 32) ? 1 : 2);
  unsigned msgDesc = 0;
  msgDesc |= getA64BTI();
  msgDesc |= op << 8;
#define A64_ATOMIC_RETURN_DATA_CONTROL_BIT 13
  msgDesc |= (dstLength ? 1 : 0) << A64_ATOMIC_RETURN_DATA_CONTROL_BIT;
  msgDesc |= ((bitwidth == 16 || bitwidth == 32) ? 0 : 1) << 12;

  // Fill remaining bits.
  FillSVMAtomicMsgDesc(bitwidth == 16, IsFloatAtomicOps(atomicOp), msgDesc);

  auto dclSizeAtLeast = [](G4_SrcRegRegion *srcRgn, unsigned int numGRFs) {
    // Verify that srcRgn contains at least as many rows as numGRFs.
    // If not, extend size of underlying dcl to accommodate as many rows as
    // numGRFs.
    auto *dcl = srcRgn->getTopDcl();
    auto totalNumRows = dcl->getNumRows();
    auto curRow = srcRgn->getRegOff();
    auto totalRowsNeeded = (curRow + numGRFs);
    if (totalRowsNeeded <= totalNumRows)
      return;
    // atomic operations use src0 (address) size based on native execution
    // size Even when SIMD size of instruction < min native execution size,
    // size of address payload needs to be based on min native execution size.
    // This means we could use r127 as src0 for a SIMD8 atomic operation. But
    // since PVC's native execution size is SIMD16, address payload size must
    // be 2 GRFs. r128 becomes OOB access, even though it may not be used.
    // In order to ensure that we don't end up reading/writing beyond last GRF,
    // we resize underlying dcl of src0 to accommodate worst case assignment.
    dcl->resizeNumRows(totalRowsNeeded);
  };
  dclSizeAtLeast(msgs[0], sizes[0]);

  if (msgs[1] == 0) {
    createSendInst(pred, dst, msgs[0], sizes[0], dstLength, instExSize, msgDesc,
                   SFID::DP_DC1, false, SendAccess::READ_WRITE, NULL, NULL,
                   instOpt, false);
  } else {
    createSplitSendInst(pred, dst, msgs[0], sizes[0], msgs[1], sizes[1],
                        dstLength, instExSize, msgDesc, SFID::DP_DC1, false,
                        SendAccess::READ_WRITE, NULL, NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

G4_SrcRegRegion *IR_Builder::getSVMOffset(G4_Operand *globalOffset,
                                          G4_SrcRegRegion *offsets,
                                          uint16_t exSize, G4_Predicate *pred,
                                          uint32_t mask) {
  G4_Declare *dcl = createSendPayloadDcl(exSize, offsets->getType());
  G4_DstRegRegion *tmp = createDstRegRegion(dcl, 1);
  createInst(pred, G4_add, 0, g4::NOSAT, g4::SIMD8, tmp, offsets, globalOffset,
             mask, true);
  if (exSize == 16) {
    // do second half of the 64-bit add
    int offset = (8 * sizeof(uint64_t)) / getGRFSize();
    auto dst = createDst(dcl->getRegVar(), offset, 0, 1, offsets->getType());
    auto src = createSrc(offsets->getBase(), offsets->getRegOff() + offset,
                         offsets->getSubRegOff(), getRegionStride1(),
                         offsets->getType());
    createInst(duplicateOperand(pred), G4_add, 0, g4::NOSAT, g4::SIMD8, dst,
               src, duplicateOperand(globalOffset), getSplitHiEMask(16, mask),
               true);
  }
  return createSrcRegRegion(dcl, getRegionStride1());
}

int IR_Builder::translateSVMGather4Inst(VISA_Exec_Size execSize,
                                        VISA_EMask_Ctrl eMask,
                                        ChannelMask chMask, G4_Predicate *pred,
                                        G4_Operand *globalOffset,
                                        G4_SrcRegRegion *offsets,
                                        G4_DstRegRegion *dst) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  vISA_ASSERT_INPUT(execSize == EXEC_SIZE_8 || execSize == EXEC_SIZE_16,
              "Only support SIMD8 or SIMD16!");

  G4_ExecSize exSize{Get_VISA_Exec_Size(execSize)};
  G4_InstOpts instOpt = Get_Gen4_Emask(eMask, exSize);

  bool useSplitSend = useSends();

  // In case non-zero global offset is specified, we need to recalculate
  // offsets.
  if (!globalOffset->isImm() || globalOffset->asImm()->getImm() != 0) {
    offsets = getSVMOffset(globalOffset, offsets, exSize,
                           duplicateOperand(pred), instOpt);
  }

  PayloadSource sources[1]; // Maximal 1 sources, offsets
  unsigned len = 0;

  sources[len].opnd = offsets;
  sources[len].numElts = exSize;
  sources[len].instOpt = instOpt;
  ++len;

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, exSize, useSplitSend, sources, len);

  SFID sfid = SFID::DP_DC1;

  unsigned FC = 0;
  // Leave sidebind scaled offset 0 as it is not used now.
  FC |= DC1_A64_UNTYPED_SURFACE_READ << 14;
  FC |= (execSize == EXEC_SIZE_8 ? MDC_SM3_SIMD8 : MDC_SM3_SIMD16) << 12;
  FC |= chMask.getHWEncoding() << 8;
  FC |= getA64BTI();

  unsigned resLen =
      (exSize / getGenxDataportIOSize()) * chMask.getNumEnabledChannels();
  if (msgs[1] == 0) {
    vISA_ASSERT(sizes[1] == 0,
                "Expect the 2nd part of the payload has zero size!");
    createSendInst(pred, dst, msgs[0], sizes[0], resLen, exSize, FC, sfid,
                   false, SendAccess::READ_ONLY, NULL, NULL, instOpt, false);
  } else {
    createSplitSendInst(pred, dst, msgs[0], sizes[0], msgs[1], sizes[1], resLen,
                        exSize, FC, sfid, false, SendAccess::READ_ONLY, NULL,
                        NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateSVMScatter4Inst(VISA_Exec_Size execSize,
                                         VISA_EMask_Ctrl eMask,
                                         ChannelMask chMask, G4_Predicate *pred,
                                         G4_Operand *globalOffset,
                                         G4_SrcRegRegion *offsets,
                                         G4_SrcRegRegion *src) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  vISA_ASSERT_INPUT(execSize == EXEC_SIZE_8 || execSize == EXEC_SIZE_16,
              "Only support SIMD8 or SIMD16!");

  G4_ExecSize exSize{Get_VISA_Exec_Size(execSize)};
  G4_InstOpts instOpt = Get_Gen4_Emask(eMask, exSize);
  bool useSplitSend = useSends();

  // In case non-zero global offset is specified, we need to recalculate
  // offsets.
  if (!globalOffset->isImm() || globalOffset->asImm()->getImm() != 0) {
    offsets = getSVMOffset(globalOffset, offsets, exSize,
                           duplicateOperand(pred), instOpt);
  }

  PayloadSource sources[2]; // Maximal 2 sources, offsets + src
  unsigned len = 0;

  sources[len].opnd = offsets;
  sources[len].numElts = exSize;
  sources[len].instOpt = instOpt;
  ++len;
  sources[len].opnd = src;
  sources[len].numElts = exSize * chMask.getNumEnabledChannels();
  sources[len].instOpt = instOpt;
  ++len;

  G4_SrcRegRegion *msgs[2] = {0, 0};
  unsigned sizes[2] = {0, 0};
  preparePayload(msgs, sizes, exSize, useSplitSend, sources, len);

  SFID sfid = SFID::DP_DC1;

  unsigned FC = 0;
  // Leave sidebind scaled offset 0 as it is not used now.
  FC |= DC1_A64_UNTYPED_SURFACE_WRITE << 14;
  FC |= (execSize == EXEC_SIZE_8 ? MDC_SM3_SIMD8 : MDC_SM3_SIMD16) << 12;
  FC |= chMask.getHWEncoding() << 8;
  FC |= getA64BTI();

  G4_DstRegRegion *dst = createNullDst(Type_UD);
  if (msgs[1] == 0) {
    vISA_ASSERT(sizes[1] == 0,
                "Expect the 2nd part of the payload has zero size!");
    createSendInst(pred, dst, msgs[0], sizes[0], 0, exSize, FC, sfid, false,
                   SendAccess::WRITE_ONLY, NULL, NULL, instOpt, false);
  } else {
    createSplitSendInst(pred, dst, msgs[0], sizes[0], msgs[1], sizes[1], 0,
                        exSize, FC, sfid, false, SendAccess::WRITE_ONLY, NULL,
                        NULL, instOpt, false);
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISASVMGather4ScaledInst(
    VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask, ChannelMask chMask,
    G4_Predicate *pred, G4_Operand *globalOffset, G4_SrcRegRegion *offsets,
    G4_DstRegRegion *dst) {
  return translateSVMGather4Inst(execSize, eMask, chMask, pred, globalOffset,
                                 offsets, dst);
}

int IR_Builder::translateVISASVMScatter4ScaledInst(
    VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask, ChannelMask chMask,
    G4_Predicate *pred, G4_Operand *globalOffset, G4_SrcRegRegion *offsets,
    G4_SrcRegRegion *src) {
  return translateSVMScatter4Inst(execSize, eMask, chMask, pred, globalOffset,
                                  offsets, src);
}
