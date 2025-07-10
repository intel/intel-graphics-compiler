/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../Timer.h"
#include "BuildIR.h"

using namespace vISA;

// translates the fence fields to binary (without shifting)
static bool encodeLscFenceSubOp(LSC_FENCE_OP fo, uint32_t &enc) {
  switch (fo) {
  case LSC_FENCE_OP_NONE:
    enc = 0;
    return true;
  case LSC_FENCE_OP_EVICT:
    enc = 1;
    return true;
  case LSC_FENCE_OP_INVALIDATE:
    enc = 2;
    return true;
  case LSC_FENCE_OP_DISCARD:
    enc = 3;
    return true;
  case LSC_FENCE_OP_CLEAN:
    enc = 4;
    return true;
  case LSC_FENCE_OP_FLUSHL3:
    enc = 5;
    return true;
  case LSC_FENCE_OP_TYPE6:
    enc = 6;
    return true;
  default:
    return false;
  }
}
static bool encodeLscFenceScope(LSC_SCOPE fs, uint32_t &enc) {
  switch (fs) {
  case LSC_SCOPE_GROUP:
    enc = 0;
    return true;
  case LSC_SCOPE_LOCAL:
    enc = 1;
    return true;
  case LSC_SCOPE_TILE:
    enc = 2;
    return true;
  case LSC_SCOPE_GPU:
    enc = 3;
    return true;
  case LSC_SCOPE_GPUS:
    enc = 4;
    return true;
  case LSC_SCOPE_SYSREL:
    enc = 5;
    return true;
  case LSC_SCOPE_SYSACQ:
    enc = 6;
    return true;
  default:
    return false;
  }
}


G4_INST *IR_Builder::translateLscFence(G4_Predicate *pred, SFID sfid,
                                       LSC_FENCE_OP fenceOp, LSC_SCOPE scope,
                                       int &status) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);
  status = VISA_SUCCESS;


  auto check = [&](bool z, const char *what) {
    if (!z) {
      criticalMsgStream() << what << "\n";
      vISA_ASSERT_INPUT(false, std::string(what));
      status = VISA_FAILURE;
    }
  };

  // NOTE: fence requires 1 register sent and 1 returned for some foolish
  // reason (synchronization requires it), so we must create dummy registers.
  // I'd prefer to use the same register, but vISA blows up
  // if we dare use the same dst as src (old? hardware restriction?),
  // so we'll splurge and use two.
  const RegionDesc *rd = getRegionStride1();

  // G4_Declare *src0DummyRegDecl = createSendPayloadDcl(getGRFSize()/4,
  // Type_UD);
  G4_Declare *src0DummyRegDecl = getBuiltinR0();
  G4_SrcRegRegion *src0Dummy =
      createSrc(src0DummyRegDecl->getRegVar(), 0, 0, rd, Type_UD);
  //
  // I don't think vISA permits same dst as src0
  // G4_Declare *dstDummyRegDecl = getBuiltinR0();
  G4_DstRegRegion *dstDummy = nullptr;
  if (!hasFenceControl()) {
    G4_Declare *dstDummyRegDecl =
        createSendPayloadDcl(getGRFSize() / 4, Type_UD);
    dstDummy = createDstRegRegion(dstDummyRegDecl, 1);
  } else {
    dstDummy = createNullDst(Type_UD);
  }

  G4_SrcRegRegion *src1NullReg = createNullSrc(Type_UD);
  //
  const int src1Len = 0; // no data needed in src1

  const G4_ExecSize execSize = g4::SIMD1;
  const G4_InstOpts instOpt = Get_Gen4_Emask(vISA_EMASK_M1_NM, execSize);

  ///////////////////////////////////////////////////////////////////////////
  uint32_t desc = 0, exDesc = 0;
  // fence requires 1 non-null register sent and 1 non-null received,
  // but the contents are undefined
  const uint32_t LSC_FENCE_OPCODE = 0x1F;
  desc |= LSC_FENCE_OPCODE; // LSC_FENCE
  desc |= 1 << 25;
  desc |= (hasFenceControl() ? 0 : 1) << 20;
  //
  uint32_t fOp = 0;
  if (!encodeLscFenceSubOp(fenceOp, fOp)) {
    check(false, "invalid fence op");
  }
  desc |= fOp << 12;
  uint32_t fsEnc = 0;
  if (!encodeLscFenceScope(scope, fsEnc)) {
    check(false, "invalid fence scope");
  }
  desc |= fsEnc << 9;

  if (sfid == SFID::UGM) {
    // special token telling EU to route the UGM fence to LSC even in
    // backup mode.  Without bit 18 set, the default behavior is for
    // the UGM fence to be rerouted to HDC when the backup mode chicken
    // bit is set.
    desc |= getOption(vISA_LSCBackupMode) << 18;
  }

  G4_SendDescRaw *msgDesc = createSendMsgDesc(sfid, desc, exDesc, src1Len,
                                              SendAccess::READ_WRITE, nullptr);
  G4_InstSend *fenceInst =
      createLscSendInst(nullptr, dstDummy, src0Dummy, src1NullReg, execSize,
                        msgDesc, instOpt, LSC_ADDR_TYPE_FLAT, 0x0, true);
  (void)fenceInst;

  return fenceInst;
}

static void generateNamedBarrier(int &status, IR_Builder &irb,
                                        G4_Predicate *prd, G4_Operand *numProds,
                                        G4_Operand *numCons,
                                        G4_Operand *barrierType,
                                        G4_Operand *barrierId) {
  // We only need three dwords but they must be GRF aligned
  // (The payload only uses HDR.2:d)
  G4_Declare *header = irb.createTempVar(3, Type_UD, irb.getGRFAlign());
  //
  // The approach here is to set all immediate values via an initial mov;
  // then copy in arguments that come from variable after the fact.
  //   HDR.2[31:24] = Num Consumers
  //   HDR.2[23:16] = Num Producers
  //   HDR.2[15:14] = BarrierType
  //   HDR.2[13:8] = [undefined]
  //   HDR.2[7:0] = Named BarrierID
  uint32_t immVal = 0;
  auto tryEncImmOp =
    [&](G4_Operand *op, int offset, uint64_t mask) {
      if (op->isImm()) {
        auto imm = op->asImm()->getImm();
        vISA_ASSERT((imm & ~mask) == 0, "invalid operand count");
        immVal |= imm << offset;
      }
      return !op->isImm();
    };
  // collect and group all immediate parameters into the initial value
  bool typeNeedsMov = tryEncImmOp(barrierType, 14, 0x3);
  bool consNeedsMov = tryEncImmOp(numCons, 24, 0xFF);
  bool prodsNeedsMov = tryEncImmOp(numProds, 16, 0xFF);
  bool barIdNeedsMov = tryEncImmOp(barrierId, 0, 0xFF);

  if (prd) {
    // if the sequence accepts a predicate and one is given
    // we must emulate
    prd = irb.duplicateOperand(prd);
    vISA_ASSERT(prd->getControl() == PRED_DEFAULT,
                "predication must be default");
    prd->setControl(G4_Predicate_Control::PRED_ANY_WHOLE);
  }

  // at the very least we need to encode the barrier type (even if 0's)
  // Start that as the value in HDR.2:ud
  //
  // Special case:
  //   if immVal is zero and barIdNeedsMov. Just create a barrierId mov as
  //   the initialization to payload.ud[2].
  if (immVal == 0 && barIdNeedsMov) {
    // Avoid redundant inst: mov payload.ud[2], 0
    // Just do:  mov payload.ud[2], barrierId:ub
    vISA_ASSERT(barrierId->isSrcRegRegion() &&
                IS_BTYPE(barrierId->getType()),
                "barrier id should be srcRegRegion with byte type");
    G4_DstRegRegion *dst = irb.createDst(header->getRegVar(), 0, 2, 1, Type_UD);
    G4_SrcRegRegion *src = barrierId->asSrcRegRegion();
    G4_INST *i =
        irb.createMov(prd, g4::SIMD1, dst, src, InstOpt_WriteEnable, true);
    i->setComments("init payload.ud[2] for prod+cons with barrierId");
    barIdNeedsMov = false;
  } else {
    G4_INST *i = irb.createMov(
        prd, g4::SIMD1, irb.createDst(header->getRegVar(), 0, 2, 1, Type_UD),
        irb.createImm(immVal, Type_UD), InstOpt_WriteEnable, true);
    i->setComments("init payload.ud[2] with all immediates");
  }

  auto isSame = [](G4_Operand *O0, G4_Operand *O1) {
    return (O0 == O1 || (O0->isSrcRegRegion() && O1->isSrcRegRegion() &&
                         *O0->asSrcRegRegion() == *O1->asSrcRegRegion()));
  };

  // For anything that was indirect (probably most things here)
  // we must move manually.
  if (prodsNeedsMov && consNeedsMov && isSame(numProds, numCons)) {
    // optimization to use SIMD2 byte move for both producer and consumer thread counts
    G4_DstRegRegion *dst = irb.createDst(header->getRegVar(), 0, 10, 1, Type_UB);
    G4_INST *i = irb.createMov(prd, g4::SIMD2, dst, numProds, InstOpt_WriteEnable, true);
    i->setComments("set producer+consumer");
    consNeedsMov = prodsNeedsMov = false;
  }
  // Explicity move in the non-immediate stragglers that come from registers
  if (barIdNeedsMov) {
    G4_DstRegRegion *dst =
      irb.createDst(header->getRegVar(), 0, 8, 1, Type_UB);
    G4_INST *i = irb.createMov(prd, g4::SIMD1, dst, barrierId, InstOpt_WriteEnable, true);
    i->setComments("set barrierId");
  }
  if (typeNeedsMov) {
    G4_Declare *tmpUD = irb.createTempVar(1, Type_UD, G4_SubReg_Align::Even_Word);
    auto tDst = irb.createDst(tmpUD->getRegVar(), 0, 0, 1, Type_UD);
    G4_INST *typeI = irb.createBinOp(nullptr, G4_shl, g4::SIMD1, tDst, barrierType,
                                   irb.createImm(6, Type_UW), InstOpt_WriteEnable, true);
    typeI->setComments("prepare barrierType(shl)");
    G4_DstRegRegion *dst = irb.createDst(header->getRegVar(), 0, 9, 1, Type_UB);
    auto tSrc =
        irb.createSrc(tmpUD->getRegVar(), 0, 0, irb.getRegionScalar(), Type_UB);
    G4_INST *i =
        irb.createMov(prd, g4::SIMD1, dst, tSrc, InstOpt_WriteEnable, true);
    i->setComments("set barrierType");
  }
  if (prodsNeedsMov) {
    G4_DstRegRegion *dst =
      irb.createDst(header->getRegVar(), 0, 10, 1, Type_UB);
    G4_INST *i = irb.createMov(prd, g4::SIMD1, dst, numProds, InstOpt_WriteEnable, true);
    i->setComments("set producer");
  }
  if (consNeedsMov) {
    G4_DstRegRegion *dst =
      irb.createDst(header->getRegVar(), 0, 11, 1, Type_UB);
    G4_INST *i = irb.createMov(prd, g4::SIMD1, dst, numCons, InstOpt_WriteEnable, true);
    i->setComments("set consumer");
  }

  int desc = (0x1 << 25) + 0x4;

  auto msgDesc = irb.createSyncMsgDesc(SFID::GATEWAY, desc);
  (void)irb.createSendInst(
      prd, G4_send, g4::SIMD1, irb.createNullDst(Type_UD),
      irb.createSrcRegRegion(header, irb.getRegionStride1()),
      irb.createImm(desc, Type_UD), InstOpt_WriteEnable, msgDesc, true);
}


void IR_Builder::generateSingleBarrier(G4_Predicate *prd) {
  // single barrier: # producer = # consumer = # threads, barrier id = 0
  // For now produce no fence
  // Number of threads per threadgroup is r0.2[31:24]
  //   mov (1) Hdr.2<1>:ud 0x0
  //   mov (2) Hdr.10<1>:ub R0.11<0;1,0>:ub
  // This SIMD2 byte move is broadcasting the thread group size
  // from the r0 header into both the producer and consumer slots.
  //   Hdr.2:d[31:24,23:16]
  G4_Declare *header = createTempVar(8, Type_UD, getGRFAlign());
  auto dst = createDst(header->getRegVar(), 0, 2, 1, Type_UD);
  uint32_t headerInitValDw2 = 0x0; // initial value for DWord2
  if (getPlatform() >= Xe2 && getOption(vISA_ActiveThreadsOnlyBarrier)) {
    headerInitValDw2 |= (1 << 8);
  }
  // Header.2:d has the following format:
  //  bits[7:0] = 0x0 (barrier id)
  //  bits[8] = active only thread barrier
  //  bits[15:14] = 0 (producer/consumer)
  //  bits[23:16] = num producers = r0.11:b (r0.2[31:24] = num threads in tg)
  //  bits[31:24] = num consumers = r0.11:b (r0.2[31:24] = num threads in tg)
  auto src = createImm(headerInitValDw2, Type_UD);
  auto inst0 = createMov(g4::SIMD1, dst, src, InstOpt_WriteEnable, true);
  if (getPlatform() >= Xe2 && getOption(vISA_ActiveThreadsOnlyBarrier)) {
    inst0->addComment("signal barrier payload init (active only)");
  } else {
    inst0->addComment("signal barrier payload init");
  }

  // copy the thread count from r0.2[31:24]; SIMD2 replacate to both bytes
  // (bits [31:24][23:16] in a single op)
  dst = createDst(header->getRegVar(), 0, 10, 1, Type_UB);
  auto src0 =
      createSrc(getBuiltinR0()->getRegVar(), 0, 11, getRegionScalar(), Type_UB);
  auto inst1 = createMov(g4::SIMD2, dst, src0, InstOpt_WriteEnable, true);
  inst1->addComment("signal barrier payload (nprods, ncons)");

  // 1 message length, 0 response length, no header, no ack
  int desc = (0x1 << 25) + 0x4;

  auto msgDesc = createSyncMsgDesc(SFID::GATEWAY, desc);
  createSendInst(prd, G4_send, g4::SIMD1, createNullDst(Type_UD),
                 createSrcRegRegion(header, getRegionStride1()),
                 createImm(desc, Type_UD), InstOpt_WriteEnable, msgDesc, true);
}

static void checkNamedBarrierSrc(G4_Operand *src, bool isBarrierId,
                                 const G4_Kernel &kernel) {
  if (src->isImm()) {
    if (isBarrierId) {
      [[maybe_unused]] uint32_t val = (uint32_t)src->asImm()->getInt();
      vISA_ASSERT(val < kernel.getMaxNumOfBarriers(), "illegal named barrier id");
    }
  } else if (src->isSrcRegRegion()) {
    vISA_ASSERT(src->asSrcRegRegion()->isScalar(),
           "barrier id should have scalar region");
    vISA_ASSERT(IS_BTYPE(src->getType()), "illegal barrier operand type");
  } else {
    vISA_ASSERT(false, "illegal barrier id operand");
  }
}

static void checkNamedBarrierType(G4_Operand *src) {
  enum class NamedBarrierType { BOTH = 0, PRODUCER = 1, CONSUMER = 2 };
  if (src->isImm()) {
    [[maybe_unused]] uint32_t val = (uint32_t)src->asImm()->getInt();
    vISA_ASSERT(val == 0 || val == 1 || val == 2, "illegal named barrier type");
  } else if (src->isSrcRegRegion()) {
    vISA_ASSERT(src->asSrcRegRegion()->isScalar(),
                "barrier type should have scalar region");
    vISA_ASSERT(IS_WTYPE(src->getType()) && IS_INT(src->getType()),
                "barrier type operand should be byte type");
  } else {
    vISA_ASSERT(false, "illegal barrier type operand");
  }
}

void IR_Builder::updateNamedBarrier(G4_Operand *barrierId) {
  if (barrierId->isImm()) {
    // Mark the barrier id is being used.
    unsigned id = (unsigned)barrierId->asImm()->getInt();
    usedBarriers.set(id, true);
  } else {
    // In order to be safe, mark all barriers are being used if the barrier
    // id is unknown to vISA. We probably won't see this in typical cases
    // as the barrier id provided by users like IGC should be an immediate
    // value.
    usedBarriers.setAll();
  }
}

int IR_Builder::translateVISANamedBarrierWait(G4_Predicate *pred,
                                              G4_Operand *barrierId) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  checkNamedBarrierSrc(barrierId, true, kernel);

  updateNamedBarrier(barrierId);

  G4_Operand *barSrc = barrierId;
  if (barrierId->isSrcRegRegion()) {
    // sync can take only flag src
    G4_Declare *flagDecl = createTempFlag(1);
    flagDecl->setSubRegAlign(Even_Word);
    createMov(pred, g4::SIMD1, createDstRegRegion(flagDecl, 1), barrierId,
              InstOpt_WriteEnable, true);
    barSrc = createSrcRegRegion(flagDecl, getRegionScalar());
  }

  // Barrier WA:Compiler must check the arrival of notification in n0.0 register
  // first before inserting the sync.bar instruction. The following instruction
  // sequence must be inserted before a sync.bar instruction:
  // r10.0 has the 5bit named barrier id, which is also copied into f1.0:uw
  // generate a mask in r10.1 with bit[barrier id] set to use it against n0.0
  //     mov(1) r10.1:ud 0x1:ud
  //     (W) shl(1) r10.1:ud r10.1:ud r10.0:ud
  //     loop:
  //     and(1)(eq) f0.0 null:ud n0.0:ud r10.1:ud
  //     (f0.0) while (1) loop
  //     (W) sync.bar f1.0:uw
  // Here insert Intrinsic::NamedBarrierWA before wait which will be expanded
  // to above instructions sequence in HWWorkaround.
  // This WA need to reserve 3 DWs:
  //   The first one for getting the barrierId
  //   The second one for generating the mask
  //   The third one for saving existing flag so that WA can use it in the loop
  if (needBarrierWA()) {
    G4_Declare *WAVarReserved = createTempVar(3, Type_UD, Even_Word, "WATemp");

    G4_DstRegRegion *dstIntrinsicInst =
        createDst(WAVarReserved->getRegVar(), 0, 0, 1, Type_UD);

    createIntrinsicInst(nullptr, Intrinsic::NamedBarrierWA, g4::SIMD1,
                        dstIntrinsicInst, duplicateOperand(barrierId), nullptr,
                        nullptr, InstOpt_WriteEnable, true);
  }

  // wait barrierId
  createInst(pred, G4_wait, nullptr, g4::NOSAT, g4::SIMD1, nullptr, barSrc,
             nullptr, InstOpt_WriteEnable, true);

  return VISA_SUCCESS;
}

int IR_Builder::translateVISANamedBarrierSignal(G4_Predicate *pred,
                                                G4_Operand *barrierId,
                                                G4_Operand *barrierType,
                                                G4_Operand *numProducers,
                                                G4_Operand *numConsumers) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  checkNamedBarrierSrc(barrierId, true /* barierId */, kernel);
  checkNamedBarrierType(barrierType);
  checkNamedBarrierSrc(numProducers, false /* numProds */, kernel);
  checkNamedBarrierSrc(numConsumers, false /* numCons */, kernel);

  updateNamedBarrier(barrierId);

  int status = VISA_SUCCESS;
  generateNamedBarrier(status, *this, pred, numProducers, numConsumers,
                              barrierType, barrierId);
  return status;
}

// create a fence instruction to the data cache
// flushParam --
//              bit 0 -- commit enable
//              bit 1-4 -- L3 flush parameters
//              bit 5 -- global/SLM
//              bit 6 -- L1 flush
//              bit 7 -- SW fence only; a scheduling barrier but does not
//              generate any code
// bit 7, if set, takes precedence over other bits
G4_INST *IR_Builder::createFenceInstructionPreLSC(G4_Predicate *prd,
                                                  uint8_t flushParam,
                                                  bool commitEnable,
                                                  bool globalMemFence,
                                                  bool isSendc = false) {
  vISA_ASSERT_INPUT(getPlatformGeneration() < PlatformGen::XE2,
         "invalid API for platform (dc0 removed from platform)");

  const uint32_t L1_FLUSH_MASK = 0x40;

  int flushBits = (flushParam >> 1) & 0xF;
  vISA_ASSERT_INPUT(!supportsLSC(), "LSC fence should be handled elsewhere");
  if (noL3Flush()) {
    // L3 flush is no longer required for image memory
    flushBits = 0;
  }

  bool L1Flush =
      (flushParam & L1_FLUSH_MASK) != 0 && !(hasSLMFence() && !globalMemFence);

  int desc = 0x7 << 14 | ((commitEnable ? 1 : 0) << 13);

  desc |= flushBits << 9;

  if (L1Flush) {
    const int L1_FLUSH_BIT_LOC = 8;
    desc |= 1 << L1_FLUSH_BIT_LOC;
  }

  G4_Declare *srcDcl = getBuiltinR0();
  G4_Declare *dstDcl = createTempVar(8, Type_UD, Any);
  G4_DstRegRegion *sendDstOpnd =
      commitEnable ? createDstRegRegion(dstDcl, 1) : createNullDst(Type_UD);
  G4_SrcRegRegion *sendSrcOpnd = createSrcRegRegion(srcDcl, getRegionStride1());
  uint8_t BTI = 0x0;

  if (hasSLMFence()) {
    // we must choose either GLOBAL_MEM_FENCE or SLM_FENCE
    BTI = globalMemFence ? 0 : 0xfe;
  }

  // commitEnable = true: msg length = 1, response length = 1, dst == src
  // commitEnable = false: msg length = 1, response length = 0, dst == null
  return createSendInst(prd, sendDstOpnd, sendSrcOpnd, 1,
                        (commitEnable ? 1 : 0), g4::SIMD8, desc, SFID::DP_DC0,
                        true, SendAccess::READ_WRITE, createImm(BTI, Type_UD),
                        nullptr, InstOpt_WriteEnable, isSendc);
}

// create a default SLM fence (no flush)
G4_INST *IR_Builder::createSLMFence(G4_Predicate *prd) {
  bool commitEnable = needsFenceCommitEnable();
  if (supportsLSC()) {
    return translateLscFence(prd, SFID::SLM, LSC_FENCE_OP_NONE,
                             LSC_SCOPE_GROUP);
  }
  return createFenceInstructionPreLSC(prd, 0, commitEnable, false, false);
}

int IR_Builder::translateVISAWaitInst(G4_Operand *mask) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  // clear TDR if mask is not null and not zero
  if (mask && !(mask->isImm() && mask->asImm()->getInt() == 0)) {
    // mov (1) f0.0<1>:uw <TDR_bits>:ub {NoMask}
    G4_Declare *tmpFlagDcl = createTempFlag(1);
    G4_DstRegRegion *newPredDef = createDstRegRegion(tmpFlagDcl, 1);
    createMov(g4::SIMD1, newPredDef, mask, InstOpt_WriteEnable, true);

    // (f0.0) and (8) tdr0.0<1>:uw tdr0.0<8;8,1>:uw 0x7FFF:uw {NoMask}
    G4_Predicate *predOpnd = createPredicate(
        PredState_Plus, tmpFlagDcl->getRegVar(), 0, PRED_DEFAULT);
    G4_DstRegRegion *TDROpnd =
        createDst(phyregpool.getTDRReg(), 0, 0, 1, Type_UW);
    G4_SrcRegRegion *TDRSrc =
        createSrc(phyregpool.getTDRReg(), 0, 0, getRegionStride1(), Type_UW);
    createInst(predOpnd, G4_and, NULL, g4::NOSAT, g4::SIMD8, TDROpnd, TDRSrc,
               createImm(0x7FFF, Type_UW), InstOpt_WriteEnable, true);
  }

  createIntrinsicInst(nullptr, Intrinsic::Wait, g4::SIMD1, nullptr, nullptr,
                      nullptr, nullptr, InstOpt_WriteEnable, true);

  return VISA_SUCCESS;
}

void IR_Builder::updateBarrier() {
  // The legacy barrier is always allocated to id 0.
  usedBarriers.set(0, true);
}

void IR_Builder::generateBarrierSend(G4_Predicate *prd) {
  updateBarrier();

  if (hasUnifiedBarrier()) {
    generateSingleBarrier(prd);
    return;
  }

  // 1 message length, 0 response length, no header, no ack
  int desc = (0x1 << 25) + 0x4;

  // get barrier id
  G4_Declare *dcl = createSendPayloadDcl(getGenxDataportIOSize(), Type_UD);

  G4_SrcRegRegion *r0_src_opnd =
      createSrc(builtinR0->getRegVar(), 0, 2, getRegionScalar(), Type_UD);

  G4_DstRegRegion *dst1_opnd = createDstRegRegion(dcl, 1);

  bool enableBarrierInstCounterBits =
      kernel.getOption(VISA_EnableBarrierInstCounterBits);
  int mask = getBarrierMask(enableBarrierInstCounterBits);

  G4_Imm *g4Imm = createImm(mask, Type_UD);

  createBinOp(prd, G4_and,
              // SPECIFY: why SIMD8?
              g4::SIMD8, dst1_opnd, r0_src_opnd, g4Imm, InstOpt_WriteEnable,
              true);

  // Generate the barrier send message
  auto msgDesc = createSyncMsgDesc(SFID::GATEWAY, desc);
  createSendInst(prd, G4_send, g4::SIMD1, createNullDst(Type_UD),
                 createSrcRegRegion(dcl, getRegionStride1()),
                 createImm(desc, Type_UD), InstOpt_WriteEnable, msgDesc, true);
}

void IR_Builder::generateBarrierWait(G4_Predicate *prd) {
  updateBarrier();

  G4_Operand *waitSrc = nullptr;
  if (!hasUnifiedBarrier()) {

    if (getPlatform() < GENX_TGLLP) {
      // before Xe: wait n0.0<0;1,0>:ud
      waitSrc =
          createSrc(phyregpool.getN0Reg(), 0, 0, getRegionScalar(), Type_UD);
    } else {
      // Xe: sync.bar null
      waitSrc = createNullSrc(Type_UD);
    }
  } else {
    if (getPlatform() >= Xe_PVC) {
      // PVC: sync.bar 0
      waitSrc = createImm(0, Type_UD);
    } else {
      // DG2: sync.bar null
      waitSrc = createNullSrc(Type_UD);
    }
  }

  // Barrrier WA: Compiler must check the arrival of notification in n0.0
  // register first before inserting the sync.bar instruction. The following
  // instructions sequence must be inserted before a sync.bar instruction.
  // Legacy barrier case:
  //    loop:
  //    and (1) (eq)f0.0 null:ud n0.0:ud 0x1:ud
  //    (f0.0) while(1) loop
  //    (W) sync.bar 0x0:uw
  // Here insert Intrinsic::BarrierWA before wait which will be expanded
  // to above instructions sequence in HWWorkaround.
  // This WA needs to reserve 1 DW for saving existing flag so that WA can
  // use it in the loop
  if (needBarrierWA()) {
    G4_Declare *WAVarReserved = createTempVar(1, Type_UD, Even_Word, "WATemp");

    G4_DstRegRegion *dstIntrinsicInst =
        createDst(WAVarReserved->getRegVar(), 0, 0, 1, Type_UD);

    createIntrinsicInst(nullptr, Intrinsic::BarrierWA, g4::SIMD1,
                        dstIntrinsicInst, nullptr, nullptr,
                        nullptr, InstOpt_WriteEnable, true);
  }

  createInst(prd, G4_wait, nullptr, g4::NOSAT, g4::SIMD1, nullptr, waitSrc,
             nullptr, InstOpt_WriteEnable, true);
}

int IR_Builder::translateVISASyncInst(ISA_Opcode opcode, unsigned int mask) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  switch (opcode) {
  case ISA_BARRIER: {
    generateBarrierSend(nullptr);
    generateBarrierWait(nullptr);
  } break;
  case ISA_SAMPLR_CACHE_FLUSH: {
       // msg length = 1, response length = 1, header_present = 1,
       // Bit 16-12 = 11111 for Sampler Message Type
       // Bit 18-17 = 11 for SIMD32 mode
    int desc = (1 << 25) + (1 << 20) + (1 << 19) + (0x3 << 17) + (0x1F << 12);

    G4_Declare *dcl = getBuiltinR0();
    G4_Declare *dstDcl = createTempVar(8, Type_UD, Any);
    G4_DstRegRegion *sendDstOpnd = createDstRegRegion(dstDcl, 1);
    G4_SrcRegRegion *sendMsgOpnd = createSrcRegRegion(dcl, getRegionStride1());

    auto msgDesc = createSyncMsgDesc(SFID::SAMPLER, desc);
    createSendInst(nullptr, G4_send, g4::SIMD8, sendDstOpnd, sendMsgOpnd,
                   createImm(desc, Type_UD), 0, msgDesc, true);

    G4_SrcRegRegion *moveSrcOpnd =
        createSrc(dstDcl->getRegVar(), 0, 0, getRegionStride1(), Type_UD);
    createMovInst(dstDcl, 0, 0, g4::SIMD8, NULL, NULL, moveSrcOpnd);
  } break;
  case ISA_WAIT: {
    // This should be handled by translateVISAWait() now
    vISA_ASSERT_UNREACHABLE("incorrect opcode");
  } break;
  case ISA_YIELD: {
    G4_INST *lastInst = instList.empty() ? nullptr : instList.back();
    if (lastInst && lastInst->opcode() != G4_label) {
      lastInst->setOptionOn(InstOpt_Switch);
    } else {
      // dummy move to apply the {switch}
      G4_SrcRegRegion *srcOpnd = createSrc(getBuiltinR0()->getRegVar(), 0, 0,
                                           getRegionScalar(), Type_UD);
      G4_DstRegRegion *dstOpnd =
          createDst(getBuiltinR0()->getRegVar(), 0, 0, 1, Type_UD);

      G4_INST *nop =
          createMov(g4::SIMD1, dstOpnd, srcOpnd, InstOpt_NoOpt, true);
      nop->setOptionOn(InstOpt_Switch);
    }
  } break;
  case ISA_FENCE: {
    const uint32_t GLOBAL_MASK = 0x20;
    struct VISAFenceMask {
      uint8_t commitEnable : 1;
      uint8_t flushICache : 1;
      uint8_t flushSCache : 1;
      uint8_t flushCCache : 1;
      uint8_t flushRWCache : 1;
      uint8_t isGlobal : 1;
      uint8_t flushL1Cache : 1;
      uint8_t SWFence : 1;
    };

    union fenceParam {
      VISAFenceMask mask;
      uint8_t data;
    };

    fenceParam fenceMask;
    fenceMask.data = mask & 0xFF;
    bool globalFence = (mask & GLOBAL_MASK) == 0;

    if (fenceMask.mask.SWFence) {
      createIntrinsicInst(nullptr, Intrinsic::MemFence, g4::SIMD1, nullptr,
                          nullptr, nullptr, nullptr, InstOpt_NoOpt, true);
    } else if (VISA_WA_CHECK(m_pWaTable, WADisableWriteCommitForPageFault)) {
      // write commit does not work under page fault
      // so we generate a fence without commit, followed by a read surface info
      // to BTI 0
      createFenceInstructionPreLSC(nullptr, (uint8_t)mask & 0xFF, false,
                                   globalFence);
      G4_Imm *surface = createImm(0, Type_UD);
      G4_Declare *zeroLOD = createTempVar(8, Type_UD, Any);
      createMovInst(zeroLOD, 0, 0, g4::SIMD8, NULL, NULL,
                    createImm(0, Type_UD));
      G4_SrcRegRegion *sendSrc =
          createSrcRegRegion(zeroLOD, getRegionStride1());
      G4_DstRegRegion *sendDst = createDstRegRegion(zeroLOD, 1);
      ChannelMask maskR = ChannelMask::createFromAPI(CHANNEL_MASK_R);
      translateVISAResInfoInst(EXEC_SIZE_8, vISA_EMASK_M1, maskR, surface,
                               sendSrc, sendDst);
    } else if (supportsLSC()) {
      // translate legacy fence into the LSC fence
      // for local fence we translate into a SLM fence with TG scope
      // for global fence we translate into a untyped and typed fence with GPU
      // scope ToDo: may need a global flag to let user control the fence scope
      if (globalFence) {
        auto fenceControl =
            supportsSampler() ? LSC_FENCE_OP_EVICT : LSC_FENCE_OP_NONE;
        if (fenceMask.mask.flushRWCache) {
          fenceControl = LSC_FENCE_OP_FLUSHL3;
        }
        translateLscFence(nullptr, SFID::UGM, fenceControl, LSC_SCOPE_GPU);
        translateLscFence(nullptr, SFID::TGM, fenceControl, LSC_SCOPE_GPU);
      } else {
        translateLscFence(nullptr, SFID::SLM, LSC_FENCE_OP_NONE,
                          LSC_SCOPE_GROUP);
      }
    } else {
      createFenceInstructionPreLSC(nullptr, (uint8_t)mask & 0xFF,
                                   (mask & 0x1) == 0x1, globalFence);
      // The move to ensure the fence is actually complete will be added at the
      // end of compilation, in Optimizer::HWWorkaround()
    }
    break;
  }
  default:
    return VISA_FAILURE;
  }

  return VISA_SUCCESS;
}

int IR_Builder::translateVISASplitBarrierInst(G4_Predicate *prd,
                                              bool isSignal) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  if (isSignal) {
    generateBarrierSend(prd);
  } else {
    generateBarrierWait(prd);
  }

  return VISA_SUCCESS;
}

