/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SendFusion.hpp"
#include "../BuildIR.h"
#include "../G4_IR.hpp"

#include <algorithm>
#include <map>

using namespace vISA;

namespace vISA {
class SendFusion {
private:
  enum {
    // Control if one pair of two sends should be considered for
    // fusion. If their distance (#instructions) is greater than
    // SEND_FUSION_MAX_SPAN, it will not be fused.
    SEND_FUSION_MAX_SPAN = 40,

    // Control how many instructions except send itself need to
    // be moved in order to move two sends together for fusion.
    SEND_FUSION_MAX_INST_TOBEMOVED = 4
  };

  FlowGraph *CFG;
  IR_Builder *Builder;
  Mem_Manager *MMgr;

  // BB that is being processed now
  G4_BB *CurrBB;

  // Dispatch mask for shader entry (1 Type_UD), One per shader.
  G4_VarBase *DMaskUD;

  // For each BB, if sr0.2 is modified, save it in this map
  std::map<G4_BB *, G4_INST *> LastSR0ModInstPerBB;

  // Flag var used within each BB (1 Type_UW).
  G4_INST *FlagDefPerBB;

  // InstToBeSinked[]/InstToBeHoisted[] keeps all instructions
  // that would be moved in order to keep two sends together
  // for fusion. Note that the 1st entry is the send instruction
  // to be moved (so the size has +1).
  int numToBeSinked;
  int numToBeHoisted;
  G4_INST *InstToBeSinked[SEND_FUSION_MAX_INST_TOBEMOVED + 1];
  G4_INST *InstToBeHoisted[SEND_FUSION_MAX_INST_TOBEMOVED + 1];

  //
  // For Both doSink() and doHoist(), all instructions to be moved
  // are within range (StartIT, EndIT), not including StartIT and EndIT.
  // All iterators (StartIT, EndIT, InsertBeforePos) remain valid.
  //
  // Note that those iterators are for instructions in CurrBB.
  void doSink(INST_LIST_ITER StartIT, INST_LIST_ITER EndIT,
              INST_LIST_ITER InsertBeforePos);
  void doHoist(INST_LIST_ITER StartIT, INST_LIST_ITER EndIT,
               INST_LIST_ITER InsertBeforePos);

  // If this optimization does any change to the code, set it to true.
  bool changed;

  // Check if this instruction is a candidate, might do simplification.
  // Return true if it is an candidate for send fusion.
  bool simplifyAndCheckCandidate(INST_LIST_ITER Iter);
  // Check if two send insts can be fused, return true if so.
  // Note that IT0 appears before IT1 in the same BB.
  bool canFusion(INST_LIST_ITER IT0, INST_LIST_ITER IT1);
  void doFusion(INST_LIST_ITER IT0, INST_LIST_ITER IT1, bool IsSink);

  // Common function for canSink()/canHoist(). It checks if
  // StartIT can sink to EndIT or EndIT can hoist to StartIT
  // based on whether isForward is true or false.
  bool canMoveOver(INST_LIST_ITER StartIT, INST_LIST_ITER EndIT,
                   bool isForward);

  // Return true if IT can be sinked to sinkToIT.
  bool canSink(INST_LIST_ITER IT, INST_LIST_ITER sinkBeforeIT) {
    return canMoveOver(IT, sinkBeforeIT, true);
  }

  // Return true if IT can be hoisted to hostToIT.
  bool canHoist(INST_LIST_ITER hoistAfterIT, INST_LIST_ITER IT) {
    return canMoveOver(hoistAfterIT, IT, false);
  }

  void initDMaskModInfo();
  void createDMask(G4_BB *bb, INST_LIST_ITER InsertBeforePos);
  void createFlagPerBB(G4_BB *bb, INST_LIST_ITER InsertBeforePos);

  void packPayload(G4_INST *FusedSend, G4_INST *Send0, G4_INST *Send1,
                   G4_BB *bb, INST_LIST_ITER InsertBeforePos);

  void unpackPayload(G4_INST *FusedSend, G4_INST *Send0, G4_INST *Send1,
                     G4_BB *bb, INST_LIST_ITER InsertBeforePos);

  G4_VarBase *getVarBase(G4_VarBase *RegVar, G4_Type Ty);
  uint32_t getFuncCtrlWithSimd16(const G4_SendDescRaw *Desc);
  void simplifyMsg(INST_LIST_ITER SendIter);
  bool isAtomicCandidate(const G4_SendDescRaw *msgDesc);

  bool WAce0Read;

public:
  SendFusion(FlowGraph *aCFG, Mem_Manager *aMMgr)
      : CFG(aCFG), Builder(aCFG->builder), MMgr(aMMgr), CurrBB(nullptr),
        DMaskUD(nullptr), FlagDefPerBB(nullptr), WAce0Read(false) {
    // Using "dmask (sr0.2) And ce0.0" for emask for each BB is not very safe
    // for the following reasons:
    //    1) shader may use vmask; or
    //    2) reading ce might be unsafe (for example, hardware issue)
    // Thus, we set WAce0Read to true to force using "cmp r0, r0" to create
    // emask always, and no dmask would be used at all!
    // WAce0Read = VISA_WA_CHECK(Builder->getPWaTable(), Wa_1406950495) ;
    WAce0Read = true;
    if (!WAce0Read) {
      initDMaskModInfo();
    }
  }

  bool run(G4_BB *BB);
};
} // namespace vISA

// Return the Function Control that is the original except SIMD Mode
// is set to SIMD16.
uint32_t SendFusion::getFuncCtrlWithSimd16(const G4_SendDescRaw *Desc) {
  uint32_t FC = Desc->getFuncCtrl();
  auto funcID = Desc->getFuncId();
  uint32_t msgType = Desc->getHdcMessageType();
  bool unsupported = false;
  if (funcID == SFID::DP_DC0) {
    switch (msgType) {
    default:
      unsupported = true;
      break;

    case DC_DWORD_SCATTERED_READ:
    case DC_DWORD_SCATTERED_WRITE:
    case DC_BYTE_SCATTERED_READ:
    case DC_BYTE_SCATTERED_WRITE:
      // bit 8 : SM2
      FC = ((FC & ~0x100) | (MDC_SM2_SIMD16 << 0x8));
      break;
    }
  } else if (funcID == SFID::DP_DC1) {
    switch (msgType) {
    default:
      unsupported = true;
      break;
    case DC1_UNTYPED_SURFACE_READ:
    case DC1_UNTYPED_SURFACE_WRITE:
      // bit13-12: SM3
      FC = ((FC & ~0x3000) | (MDC_SM3_SIMD16 << 12));
      break;
    case DC1_UNTYPED_ATOMIC:
    case DC1_UNTYPED_FLOAT_ATOMIC:
    case DC1_UNTYPED_HALF_FLOAT_ATOMIC:
    case DC1_UNTYPED_HALF_INTEGER_ATOMIC:
      // bit12: SM2R
      FC = ((FC & ~0x1000) | (MDC_SM2R_SIMD16 << 12));
      break;
    }
  } else if (funcID == SFID::DP_DC2) {
    switch (msgType) {
    default:
      unsupported = true;
      break;
    case DC2_BYTE_SCATTERED_READ:
    case DC2_BYTE_SCATTERED_WRITE:
      // bit 8 : SM2
      FC = ((FC & ~0x100) | (MDC_SM2_SIMD16 << 0x8));
      break;

    case DC2_UNTYPED_SURFACE_READ:
    case DC2_UNTYPED_SURFACE_WRITE:
      // bit13-12: SM3
      FC = ((FC & ~0x3000) | (MDC_SM3_SIMD16 << 12));
      break;
    }
  }

  if (unsupported) {
    vISA_ASSERT_UNREACHABLE("Unsupported message!");
  }
  return FC;
}

bool SendFusion::isAtomicCandidate(const G4_SendDescRaw *msgDesc) {
  auto funcID = msgDesc->getSFID();
  if (funcID != SFID::DP_DC1) {
    return false;
  }

  // Right now, the following atomic messages are DW per simd-lane.
  uint16_t msgType = msgDesc->getHdcMessageType();
  bool intAtomic = true; // true: int; false : float
  switch (msgType) {
  default:
    return false;
  case DC1_UNTYPED_ATOMIC:
    break;
  case DC1_UNTYPED_FLOAT_ATOMIC:
    intAtomic = false;
    break;
  case DC1_UNTYPED_HALF_INTEGER_ATOMIC:
    break;
  case DC1_UNTYPED_HALF_FLOAT_ATOMIC:
    intAtomic = false;
    break;
  // DG2 has simd16 A64 atomic using LSC, and seems no legacy simd16 A64.
  // Thus, the following will not work. For this reason, turn it off.
  // (Need to revisit this later when we want to do fusion for LSC messages.)
  case DC1_A64_ATOMIC:
  case DC1_A64_UNTYPED_HALF_INTEGER_ATOMIC:
    if (Builder->getPlatform() != Xe_XeHPSDV)
      return false;
    break;
  case DC1_A64_UNTYPED_FLOAT_ATOMIC:
  case DC1_A64_UNTYPED_HALF_FLOAT_ATOMIC:
    if (Builder->getPlatform() != Xe_XeHPSDV)
      return false;
    intAtomic = false;
    break;
  }

  // Had right atomic type, now check AtomicOp
  uint16_t atomicOp = msgDesc->getHdcAtomicOp();
  if (intAtomic) {
    switch (atomicOp) {
    default:
      return false;
    case GEN_ATOMIC_AND:
    case GEN_ATOMIC_OR:
    case GEN_ATOMIC_XOR:
    case GEN_ATOMIC_INC:
    case GEN_ATOMIC_DEC:
    case GEN_ATOMIC_ADD:
    case GEN_ATOMIC_SUB:
    case GEN_ATOMIC_REVSUB:
    case GEN_ATOMIC_IMAX:
    case GEN_ATOMIC_IMIN:
    case GEN_ATOMIC_UMAX:
    case GEN_ATOMIC_UMIN:
    case GEN_ATOMIC_PREDEC:
      break;
    }
  } else {
    switch (atomicOp) {
    default:
      return false;
    case GEN_ATOMIC_FMAX:
    case GEN_ATOMIC_FMIN:
    case GEN_ATOMIC_FADD:
    case GEN_ATOMIC_FSUB:
      break;
    }
  }
  return true;

  // Need to check if it is packed half integer/float ?
}

// We will do send fusion for a few messages. Those messages all
// have DW-sized address for each lane, thus address payload is
// 1 GRF for exec_size=8 (no A64 messages for now).
//
// The optimization is performed for the following cases:
//    1) [(w)] send(8) + send(8) --> (W&flag) send(16), and
//          Either noMask or not. When no NoMask, the shader
//          must be SIMD8.
//    2) (w) send(1|2|4) -> (W) send (2|4|8).
//          Must have NoMask.
//          Note that this case can be performed for shader of
//          SIMD8 or SIMD16, SIMD32.
//
bool SendFusion::simplifyAndCheckCandidate(INST_LIST_ITER Iter) {
  G4_INST *I = *Iter;
  // For now, we will handle a few simple messages.
  // If needed, more messages can be handled later.
  G4_opcode opc = I->opcode();
  if ((opc != G4_send && opc != G4_sends) || I->getExecSize() > g4::SIMD8 ||
      I->getPredicate() != nullptr ||
      !(I->is1QInst() || I->isWriteEnableInst())) {
    return false;
  }

  // If DMask is modified (for example, CPS), say in BB,
  // only handle sends that are after the modification in BB.
  //   BB
  //      send   // (1)
  //      send   // (2)
  //      sr0.2 = ...
  //      send   // (3)
  //      send   // (4)
  // only send 3 and 4 are handled. All sends before "sr0.2 = "
  // are skipped!

  if (LastSR0ModInstPerBB.count(CurrBB)) {
    G4_INST *sr0ModInst = LastSR0ModInstPerBB[CurrBB];
    if (sr0ModInst->getLocalId() > I->getLocalId()) {
      return false;
    }
  }

  const G4_SendDescRaw *msgDesc = I->getMsgDescRaw();
  if (!msgDesc || !msgDesc->isHDC() || msgDesc->isHeaderPresent() ||
      msgDesc->getSti() != nullptr) {
    return false;
  }

  // If exec_size=1|2|4, make sure that it is noMask and its
  // data payload/response is 1 GRF.
  uint16_t msgLen = msgDesc->MessageLength();
  uint16_t rspLen = msgDesc->ResponseLength();
  uint16_t extMsgLen = msgDesc->extMessageLength();
  if (I->getExecSize() < g4::SIMD8 &&
      !(I->isWriteEnableInst() &&
        ((msgDesc->isDataPortWrite() && (msgLen + extMsgLen) == 2) ||
         (msgDesc->isDataPortRead() && msgLen == 1 && rspLen == 1)))) {
    return false;
  }

  // For write messages:
  //   unless we can prove there are no aliases of two sends's address payload,
  //   we will not be able to do fusion as hardware does not have deterministic
  //   behavior if the same address appear more than once. For example,
  //
  //     send (8)  (a1_0, C, a1_2, ..., a1_7) (d1_0, d1_1, d1_2, ..., d1_7)
  //     send (8)  (a2_0, C, a2_2, ..., a2_7) (d2_0, d2_1, d2_2, ..., d2_7)
  //
  //   Both sends have the same addr 'C' in lane 1, and C's value is d2_1 after
  //   two sends. However, if they are fused, they become:
  //
  //     send (16) (a1_0, C,    ..., a2_0, C,    ..., a2_7)
  //               (d1_0, d1_1, ..., d2_0, d2_1, ..., d2_7)
  //
  //   The hardware cannot guarantee that d2_1 will be the value written to addr
  //   'C'. Thus, we have to disallow fusion for writes
  //
  // But we can do it for atomic messages if certain conditions meet:
  // (Checked in isAtomicCandidate().)
  //    1. Can fuse if no return value.
  //       Atomic messages has both read/write. Let us take a look at atomic_add
  //       for example: let's assume atomic_add returns the old value and does
  //       updating.
  //
  //          location at p :  10 (original value)
  //          x = atomic_add p, 1
  //          y = atomic_add p, 2
  //       then, x = 10  & y = 11
  //
  //       If fused them, it becomes:
  //          {x, y} = atomic_add {p, p}, {1, 2}
  //            it's possible that we have x = 12 & y = 10 as the 2nd atomic
  //            operations could be performed first. This does change the
  //            behavior of the program.
  //
  //       Since we don't know if two sends share the same address, we will
  //       conservatively avoid fusing two sends with return values for now.
  //    2. Assume unsafe math is present for float atomic.
  //       As two sends might share the same location, and fused send might
  //       change the order of float atomic operations.  We need to have
  //       unsafe-math to perform fusing legally.
  //
  bool isAtomicCand = false;
  if (Builder->getOption(vISA_EnableAtomicFusion)) {
    isAtomicCand = isAtomicCandidate(msgDesc);
  }

  // -enableWriteFusion is for testing only, as it's not safe
  if ((!Builder->getOption(vISA_EnableWriteFusion)) &&
      ((!isAtomicCand && msgDesc->isDataPortWrite()) ||
       (isAtomicCand && rspLen > 0))) {
    return false;
  }

  // If rspLen > 1, skip.
  if (rspLen > 1) {
    return false;
  }

  // Send might have a0 as its descriptor, if we know a0 is
  // a compile-time constant, replace a0 with the constant.
  simplifyMsg(Iter);

  // Make sure send's operands are direct GRF operands!
  G4_DstRegRegion *dst = I->getDst();
  G4_SrcRegRegion *src0 = I->getSrc(0)->asSrcRegRegion();
  G4_SrcRegRegion *src1 =
      I->isSplitSend() ? I->getSrc(1)->asSrcRegRegion() : nullptr;
  if ((dst && dst->isIndirect()) || (src0 && src0->isIndirect()) ||
      (src1 && src1->isIndirect())) {
    return false;
  }

  // For now, handle constant descriptor (and ext Desc)
  G4_Operand *descOpnd = I->isSplitSend() ? I->getSrc(2) : I->getSrc(1);
  G4_Operand *extDescOpnd = I->isSplitSend() ? I->getSrc(3) : nullptr;
  if (!descOpnd->isImm() || (extDescOpnd && !extDescOpnd->isImm())) {
    return false;
  }

  // Only handling the following messages that have DW as data element.
  // Untyped with up to 4 DW per lane is handled.

  // special handling of atomic
  if (isAtomicCand) {
    return true;
  }

  // only enable this on a subset of the HDC messages
  auto funcID = msgDesc->getSFID();
  switch (funcID) {
  case SFID::DP_DC0:
    switch (msgDesc->getHdcMessageType()) {
    case DC_DWORD_SCATTERED_READ:
    case DC_DWORD_SCATTERED_WRITE:
    case DC_BYTE_SCATTERED_READ:
    case DC_BYTE_SCATTERED_WRITE:
      return true;
    }
    break;
  case SFID::DP_DC1:
    switch (msgDesc->getHdcMessageType()) {
    case DC1_UNTYPED_SURFACE_READ:
    case DC1_UNTYPED_SURFACE_WRITE:
      return true;
    }
    break;
  case SFID::DP_DC2:
    switch (msgDesc->getHdcMessageType()) {
    case DC2_BYTE_SCATTERED_READ:
    case DC2_BYTE_SCATTERED_WRITE:
    case DC2_UNTYPED_SURFACE_READ:
    case DC2_UNTYPED_SURFACE_WRITE:
      return true;
    }
    break;
  default:
    break;
  }

  return false;
}

G4_VarBase *SendFusion::getVarBase(G4_VarBase *Var, G4_Type Ty) {
  G4_RegVar *RegVar = Var->asRegVar();
  G4_Declare *Dcl = RegVar->getDeclare();
  G4_Type DclTy = Dcl->getElemType();
  if (Dcl->getElemType() == Ty) {
    return Var;
  }
  int16_t sz = TypeSize(DclTy) * Dcl->getNumElems();
  int16_t elts = sz / TypeSize(Ty);
  G4_Declare *newDcl = Builder->createTempVar(elts, Ty, Any);
  newDcl->setAliasDeclare(Dcl->getRootDeclare(), Dcl->getAliasOffset());
  return newDcl->getRegVar();
}

// For Send's desc, it may have a0 as its descriptor. If so,
// simplifyMsg() tries to replace a0 with a constant if possible.
void SendFusion::simplifyMsg(INST_LIST_ITER SendIter) {
  G4_InstSend *Send = (*SendIter)->asSendInst();
  Gen4_Operand_Number opn = Send->isSplitSend() ? Opnd_src2 : Opnd_src1;
  G4_Operand *descOpnd = Send->getSrc(opn - 1);
  if (descOpnd->isImm()) {
    return;
  }

  // Need to find the bti from following pattern:
  //   (W) mov(1) T6(0, 0)<1>:d 0x2:uw
  //   (W) add(1) a0.0<1>:ud T6(0,0)<0;1,0> : ud 0x2110800:ud
  //       send(8) V84(0, 0)<1>:f V82(0,0)<1;1,0>:ud a0.0<0;1,0>:ud
  // 0x2 is the bti.
  int nDefs = 0;
  G4_INST *addI = nullptr;
  for (auto DI = Send->def_begin(), DE = Send->def_end(); DI != DE; ++DI) {
    if (DI->second == opn) {
      addI = DI->first;
      ++nDefs;
    }
  }

  if (nDefs != 1 || addI->opcode() != G4_add || !addI->getSrc(1)->isImm() ||
      !addI->isWriteEnableInst() || addI->getExecSize() != g4::SIMD1) {
    return;
  }

  G4_INST *movI = nullptr;
  nDefs = 0;
  for (auto DI = addI->def_begin(), DE = addI->def_end(); DI != DE; ++DI) {
    if (DI->second == Opnd_src0) {
      movI = DI->first;
      ++nDefs;
    }
  }
  if (nDefs != 1 || movI->opcode() != G4_mov || !movI->getSrc(0)->isImm() ||
      !movI->isWriteEnableInst() || movI->getExecSize() != g4::SIMD1) {
    return;
  }

  // Sanity check
  G4_SendDescRaw *desc = Send->getMsgDescRaw();
  if (!desc)
    return;
  G4_Operand *bti = desc->getBti();
  if ((bti && bti->getTopDcl() != movI->getDst()->getTopDcl()) ||
      (addI->getSrc(1)->asImm()->getInt() != desc->getDesc())) {
    return;
  }

  uint32_t descImm = (uint32_t)(movI->getSrc(0)->asImm()->getInt() +
                                addI->getSrc(1)->asImm()->getInt());
  G4_Imm *newImm = Builder->createImm(descImm, descOpnd->getType());
  Send->setSrc(newImm, opn - 1);
  Send->removeDefUse(opn);

  // Need to re-create descriptor for this send
  G4_SendDesc *newDesc = Builder->createSendMsgDesc(
      desc->getFuncId(), descImm, desc->getExtendedDesc(),
      desc->extMessageLength(), desc->getAccess(), desc->getBti(),
      desc->getExecSize());
  Send->setMsgDesc(newDesc);

  // If addI or movI is dead, remove them.
  // Normally, addI and movI are right before Send and
  // remove them using iterator is more efficient. So
  // we check if the previous iterators refer to them,
  // if so, use iterators to remove them.
  // (As we found addI and movI in this BB, decreasing
  //  SendIter twice should be safe and no need to check
  //  if the iterator refers to the begin().)
  INST_LIST_ITER I1st = SendIter;
  --I1st;
  INST_LIST_ITER I2nd = I1st;
  --I2nd;
  if (addI->useEmpty()) {
    addI->removeAllDefs();
    if (addI == *I1st) {
      CurrBB->erase(I1st);
    } else {
      CurrBB->remove(addI);
    }
  }
  if (movI->useEmpty()) {
    movI->removeAllDefs();
    if (movI == *I2nd) {
      CurrBB->erase(I2nd);
    } else {
      CurrBB->remove(movI);
    }
  }

  changed = true;
}

// Note that IT0 appears prior to IT1 in the same BB.
// Return true if they can be fused.
bool SendFusion::canFusion(INST_LIST_ITER IT0, INST_LIST_ITER IT1) {
  G4_INST *I0 = *IT0;
  G4_INST *I1 = *IT1;
  G4_opcode opc = I0->opcode();
  vISA_ASSERT((opc == G4_send || opc == G4_sends) && opc == I1->opcode(),
         "Arguments to canFusion must be the same kind of Send Messages!");

  // Current implementation uses split send to replace two sends. For
  // simplicity, here make sure their payload does not overlap!
  if (opc == G4_send) {
    G4_SrcRegRegion *s0 = I0->getSrc(0)->asSrcRegRegion();
    G4_SrcRegRegion *s1 = I1->getSrc(0)->asSrcRegRegion();
    if (s0->compareOperand(s1, *Builder) != Rel_disjoint) {
      return false;
    }
  }

  // Here two descriptors should be constant, and we fuse
  // the two sends only if their descriptors are identical.
  //
  // To be able to fuse two sends, the following conditions
  // must be met :
  //
  //   1. no fusion if there is RAW
  //      x = load y
  //      z = load x
  //   2. it's okay to fuse if there is WAW
  //      x = load (8) y
  //      x = load (8) z
  //     --> new_x = load (16) (y,z);
  //         x = new_x.1stHalf;
  //         x = new_x.2ndHalf  (later optimization will clean this up)
  //   3. it's okay to fuse if there is WAR
  //      x = load (8) y
  //      y = load (8) z
  //      --> new_x = load (16) (y,z);
  //          x = new_x.1stHalf
  //          y = new_x.2ndHalf
  //
  //  ***Thus, ONLY RAW CANNOT BE FUSED ***
  //
  //    Note that write is not allowed for fusion as the following might
  //    have an unknown behavior if fused:
  //        store (8) x, data0
  //        store (8) x, data1
  //      if fused, they become:
  //        store (16) {x, x} {data0, data1}
  //
  //        and hardware does not have deterministic behavior about which data,
  //        data0 or data1, will be stored into x! For this reason, no write
  //        will be fused if they have common address. Since we don't know (for
  //        now) if addresses of two sends can point to the same location, we
  //        just conservatively do not fuse any write messages.
  //
  // Atomic messages:
  //    As only no-return-value atomic can be fused, RAW will be false always.
  //
  G4_SendDescRaw *desc0 = I0->getMsgDescRaw();
  G4_SendDescRaw *desc1 = I1->getMsgDescRaw();
  if (!desc0 || !desc1)
    return false;
  bool fusion = I0->getOption() == I1->getOption() &&
                (desc0->getDesc() == desc1->getDesc() &&
                 desc0->getExtendedDesc() == desc1->getExtendedDesc()) &&
                !I1->isRAWdep(I0);
  return fusion;
}

// canMoveOver() : common function used for sink and hoist.
//   Check if StartIT can sink to EndIT (right before EndIT) :  isForward ==
//   true. Check if EndIT can hoist to StartIT (right after StartIT) : isForward
//   == false.
// Return true if so, false otherwise.
bool SendFusion::canMoveOver(INST_LIST_ITER StartIT, INST_LIST_ITER EndIT,
                             bool isForward) {
  if (isForward) {
    numToBeSinked = 0;
  } else {
    numToBeHoisted = 0;
  }

  G4_INST *Inst_first = *StartIT;
  G4_INST *Inst_last = *EndIT;
  if (Inst_first == Inst_last) {
    return true;
  }

  int lid_first = Inst_first->getLocalId();
  int lid_last = Inst_last->getLocalId();
  vISA_ASSERT(lid_first <= lid_last, "Wrong inst position to sink to!");
  int span = lid_last - lid_first;
  if (span >= SEND_FUSION_MAX_SPAN) {
    return false;
  }

  bool movable = true;
  G4_INST *moveInst = (isForward ? Inst_first : Inst_last);
  G4_INST *destSend = (isForward ? Inst_last : Inst_first);
  INST_LIST_ITER II = (isForward ? StartIT : EndIT);
  INST_LIST_ITER IE = (isForward ? EndIT : StartIT);

  // Note that the send instruction is the 1st entry.
  int numToBeMoved = 1;
  G4_INST **toBeMoved = isForward ? InstToBeSinked : InstToBeHoisted;
  toBeMoved[0] = moveInst;

  for (isForward ? ++II : --II; II != IE; isForward ? ++II : --II) {
    G4_INST *tmp = *II;
    for (int i = 0; i < numToBeMoved; ++i) {
      // Here check if instToBeMoved and tmp
      // are dependent upon each other no matter
      // if instToBEMoved appears before or after
      // tmp in the BB.
      G4_INST *instToBeMoved = toBeMoved[i];
      if (instToBeMoved->isWARdep(tmp) || instToBeMoved->isWAWdep(tmp) ||
          instToBeMoved->isRAWdep(tmp)) {
        movable = false;
      }

      if (!movable) {
        if (numToBeMoved <= SEND_FUSION_MAX_INST_TOBEMOVED &&
            !tmp->isWARdep(destSend) && !tmp->isWAWdep(destSend) &&
            !tmp->isRAWdep(destSend)) {
          movable = true;
          toBeMoved[numToBeMoved] = tmp;
          ++numToBeMoved;
        }
        break;
      }
    }
    if (!movable) {
      break;
    }
  }

  if (movable) {
    if (isForward) {
      numToBeSinked = numToBeMoved;
    } else {
      numToBeHoisted = numToBeMoved;
    }
  }
  return movable;
}

// doSink() sinks instructions InstToBeSinked[1 : numToBeSinked-1].
//
// Note that StartIT, EndIT, and InsertBeforePos remain valid after
// invoking this function.
void SendFusion::doSink(INST_LIST_ITER StartIT, INST_LIST_ITER EndIT,
                        INST_LIST_ITER InsertBeforePos) {
  if (numToBeSinked > 1) {
    int j = 1;
    G4_INST *Inst = InstToBeSinked[j];
    for (INST_LIST_ITER IT = StartIT; IT != EndIT;) {
      G4_INST *tmp = *IT;
      if (tmp == Inst) {
        INST_LIST_ITER tmpIT = IT;
        ++IT;
        CurrBB->erase(tmpIT);

        ++j;
        if (j == numToBeSinked) {
          break;
        }
        Inst = InstToBeSinked[j];
      } else {
        ++IT;
      }
    }
    vISA_ASSERT(j == numToBeSinked,
           "Internal Error(SendFusion) : Instructions not in the list!");

    for (int i = 1; i < numToBeSinked; ++i) {
      CurrBB->insertBefore(InsertBeforePos, InstToBeSinked[i]);
    }
  }
}

// InstToBeHoisted[numToBeHosted-1 : 1] has instructions to be hoisted
// in the reverse order. Thus InstToBeHoisted[numToBeHoisted-1] will
// appear first, then InstToBeHoisted[numToBeHoisted-2], ..., and finally,
// InstToBeHoisted[1].
//
// Note that StartIT, EndIT, and InsertBeforePos remain valid after
// invoking this function.
void SendFusion::doHoist(INST_LIST_ITER StartIT, INST_LIST_ITER EndIT,
                         INST_LIST_ITER InsertBeforePos) {
  if (numToBeHoisted > 1) {
    int j = numToBeHoisted - 1;
    G4_INST *Inst = InstToBeHoisted[j];
    for (INST_LIST_ITER IT = StartIT; IT != EndIT;) {
      G4_INST *tmp = *IT;
      if (tmp == Inst) {
        INST_LIST_ITER tmpIT = IT;
        ++IT;
        CurrBB->erase(tmpIT);

        --j;
        if (j == 0) {
          break;
        }
        Inst = InstToBeHoisted[j];
      } else {
        ++IT;
      }
    }
    vISA_ASSERT(j == 0,
           "Internal Error(SendFusion) : Instructions not in the list!");

    for (int i = numToBeHoisted - 1; i > 0; --i) {
      CurrBB->insertBefore(InsertBeforePos, InstToBeHoisted[i]);
    }
  }
}

// Packing payload:
//    Given the following (untyped surface write, resLen=0, msgLen=1,
//    extMsgLen=2):
//      Send0:   sends (8|M0)  null r10 r12  0x8C   0x02026CFF
//      Send1:   sends (8|M0)  null r40 r42  0x8C   0x02026CFF
//
//    PackPayload() does:
//      mov (8) r18.0<1>:ud r10.0<8;8,1>:ud
//      mov (8) r19.0<1>:ud r40.0<8;8,1>:ud
//      mov (8) r20.0<1>:ud r12.0<8;8,1>:ud
//      mov (8) r21.0<1>:ud r42.0<8;8,1>:ud
//      mov (8) r22.0<1>:ud r13.0<8;8,1>:ud
//      mov (8) r23.0<1>:ud r43.0<8;8,1>:ud
//
//    With those mov instructions, the new send will be:
//     (untyped surface write, resLen=0, msgLen=2, extMsgLen=4)
//     (W&f0.0)) sends (16|M0)  null r18 r20  0x10C 0x04025CFF
//
// Note that f0.0 is created in doFusion(). Check it out for details.
//
void SendFusion::packPayload(G4_INST *FusedSend, G4_INST *Send0, G4_INST *Send1,
                             G4_BB *bb, INST_LIST_ITER InsertBeforePos) {
  // Both Send0 and Send1 have the same MsgDesc.
  G4_ExecSize execSize = Send0->getExecSize();
  const G4_SendDescRaw *origDesc = Send0->getMsgDescRaw();
  vISA_ASSERT(origDesc, "expected raw descriptor");
  if (!origDesc) {
    return;
  }

  int option = Send0->getOption();
  int16_t msgLen = origDesc->MessageLength();
  int16_t extMsgLen = origDesc->extMessageLength();

  // mov (ES) DVar<D_regoff, D_sregoff>:Ty<1> SVar<S_regoff,
  // S_sregoff>:Ty(1;1,0)
  auto copyRegOpnd = [&](G4_VarBase *DVar, G4_VarBase *SVar, G4_Type Ty,
                         G4_ExecSize ES, int16_t D_regoff, int16_t D_sregoff,
                         int16_t S_regoff, int16_t S_sregoff) -> G4_INST * {
    G4_SrcRegRegion *S = Builder->createSrc(SVar, S_regoff, S_sregoff,
                                            Builder->getRegionStride1(), Ty);
    G4_DstRegRegion *D = Builder->createDst(DVar, D_regoff, D_sregoff, 1, Ty);
    G4_INST *nInst = Builder->createMov(ES, D, S, option, false);
    bb->insertBefore(InsertBeforePos, nInst);
    return nInst;
  };

  // Temperaries for address payload
  G4_Type Ty = origDesc->isA64Message() ? Type_UQ : Type_UD;
  G4_VarBase *Dst0 = FusedSend->getOperand(Opnd_src0)->getBase();
  G4_SrcRegRegion *S0 = Send0->getOperand(Opnd_src0)->asSrcRegRegion();
  G4_VarBase *V0 = getVarBase(S0->getBase(), Ty);
  G4_SrcRegRegion *S1 = Send1->getOperand(Opnd_src0)->asSrcRegRegion();
  G4_VarBase *V1 = getVarBase(S1->getBase(), Ty);
  int16_t d_roff = 0;
  int16_t s0_roff = S0->getRegOff();
  int16_t s1_roff = S1->getRegOff();

  // Special case for exec_size = 1|2|4
  if (execSize < g4::SIMD8) {
    vISA_ASSERT((origDesc->isDataPortWrite() ||
            (msgLen == 1 && origDesc->ResponseLength() == 1)),
           "Internal Error (SendFusion): unexpected read message!");
    vISA_ASSERT((origDesc->isDataPortRead() || (msgLen + extMsgLen == 2)),
           "Internal Error (SendFusion): unexpected write message!");

    /// Address payload size (in unit of GRF)
    //    A64: 2*execSize UQ (1 GRF for execSize=1|2; 2 GRF for execSize=4)
    //    Otherwise: 2*execSize DW, which is 1 GRF.
    G4_INST *Inst0 = copyRegOpnd(Dst0, V0, Ty, execSize, d_roff, 0, s0_roff, 0);
    G4_INST *Inst1;
    if (origDesc->isA64Message() && execSize == 4) {
      Inst1 = copyRegOpnd(Dst0, V1, Ty, execSize, d_roff + 1, 0, s1_roff, 0);
    } else {
      Inst1 = copyRegOpnd(Dst0, V1, Ty, execSize, d_roff, execSize, s1_roff, 0);
    }

    // Update DefUse
    Inst0->addDefUse(FusedSend, Opnd_src0);
    Send0->copyDef(Inst0, Opnd_src0, Opnd_src0, true);
    Inst1->addDefUse(FusedSend, Opnd_src0);
    Send1->copyDef(Inst1, Opnd_src0, Opnd_src0, true);

    if (msgLen <= 1 && extMsgLen == 0) {
      // No source payload, done!
      return;
    }

    // Source payload
    //    Either in src0 or src1, but not both as execSize <= 4
    Ty = Type_UD; // source payload
    Gen4_Operand_Number opn = (msgLen > 1) ? Opnd_src0 : Opnd_src1;
    G4_SrcRegRegion *Reg0 = Send0->getOperand(opn)->asSrcRegRegion();
    G4_SrcRegRegion *Reg1 = Send1->getOperand(opn)->asSrcRegRegion();
    G4_VarBase *Var0 = getVarBase(Reg0->getBase(), Ty);
    G4_VarBase *Var1 = getVarBase(Reg1->getBase(), Ty);
    G4_VarBase *Dst = FusedSend->getOperand(opn)->getBase();
    d_roff = 0;
    s0_roff = Reg0->getRegOff();
    s1_roff = Reg1->getRegOff();
    if (opn == Opnd_src0) {
      // source payload follows address payload in Opnd_src0.
      d_roff += ((origDesc->isA64Message() && execSize == 4) ? 2 : 1);
      s0_roff += 1;
      s1_roff += 1;
    }

    Inst0 = copyRegOpnd(Dst, Var0, Ty, execSize, d_roff, 0, s0_roff, 0);
    Inst1 = copyRegOpnd(Dst, Var1, Ty, execSize, d_roff, execSize, s1_roff, 0);

    // Update DefUse
    Inst0->addDefUse(FusedSend, opn);
    Send0->copyDef(Inst0, opn, Opnd_src0, true);
    Inst1->addDefUse(FusedSend, opn);
    Send1->copyDef(Inst1, opn, Opnd_src0, true);

    return;
  }

  // Now, execSize = 8
  // the number of grf for address payload in the original send
  const int16_t addrLen = (origDesc->isA64Message() ? 2 : 1);

  ///
  /// 1. copy address payload
  ///
  G4_INST *Inst0 = copyRegOpnd(Dst0, V0, Ty, execSize, d_roff, 0, s0_roff, 0);
  G4_INST *Inst1 =
      copyRegOpnd(Dst0, V1, Ty, execSize, d_roff + addrLen, 0, s1_roff, 0);

  // Update DefUse
  Inst0->addDefUse(FusedSend, Opnd_src0);
  Send0->copyDef(Inst0, Opnd_src0, Opnd_src0, true);
  Inst1->addDefUse(FusedSend, Opnd_src0);
  Send1->copyDef(Inst1, Opnd_src0, Opnd_src0, true);

  ///
  /// 2. Copy source payload
  //     Using a loop of count 2 for handing both Msg & extMsg payload.

  Ty = Type_UD; // source payload
  vASSERT(msgLen >= addrLen);
  int16_t remMsgLen = (int16_t)(msgLen - addrLen);
  int16_t numMov[2] = {remMsgLen, extMsgLen};

  for (int j = 0; j < 2; ++j) {
    int32_t nMov = numMov[j];
    if (nMov <= 0)
      continue;
    Gen4_Operand_Number opn = (j == 0 ? Opnd_src0 : Opnd_src1);
    G4_SrcRegRegion *Reg0 = Send0->getOperand(opn)->asSrcRegRegion();
    G4_SrcRegRegion *Reg1 = Send1->getOperand(opn)->asSrcRegRegion();
    G4_VarBase *Var0 = getVarBase(Reg0->getBase(), Ty);
    G4_VarBase *Var1 = getVarBase(Reg1->getBase(), Ty);
    G4_VarBase *Dst = FusedSend->getOperand(opn)->getBase();
    int16_t Off = 0;
    int16_t Off0 = Reg0->getRegOff();
    int16_t Off1 = Reg1->getRegOff();
    if (j == 0) {
      // source payload follows address payload in Opnd_src0
      Off0 += addrLen;
      Off1 += addrLen;
      Off += (2 * addrLen);
    } else {
      d_roff = 0;
    }

    for (int i = 0; i < nMov; ++i) {
      // copy operands of both send0 and send1 to Dst
      G4_INST *I0 =
          copyRegOpnd(Dst, Var0, Ty, execSize, Off + 2 * i, 0, Off0 + i, 0);
      G4_INST *I1 =
          copyRegOpnd(Dst, Var1, Ty, execSize, Off + 2 * i + 1, 0, Off1 + i, 0);

      // Update DefUse
      I0->addDefUse(FusedSend, opn);
      Send0->copyDef(I0, opn, Opnd_src0, true);
      I1->addDefUse(FusedSend, opn);
      Send1->copyDef(I1, opn, Opnd_src0, true);
    }
  }
}

// unpackPayload does the following for read messages.
//   Given the following (untyped surface read, resLen=2, msgLen=1)
//     Send0: send (8|M0) r8:f   r7   0xC  0x02206CFF
//     Send0: send (8|M0) r21:f  r10  0xC  0x02206CFF
//
//   Assuming the fused send is as follows:
//     (untyped surface read, resLen=4, msgLen=1, extMsgLen=1)
//     (W&f0.0) sends (16|M0) r16:f  r7  r10  0x4C  0x02405CFF
//   (Note: check doFusion about f0.0)
//
//   Unpacking code is:
//     mov (8|M0) r8.0<1>:f  r16.0<8;8,1>:f
//     mov (8|M0) r9.0<1>:f  r18.0<8;8,1>:f
//     mov (8|M0) r21.0<1>:f r17.0<8;8,1>:f
//     mov (8|M0) r22.0<1>:f r19.0<8;8,1>:f
//
void SendFusion::unpackPayload(G4_INST *FusedSend, G4_INST *Send0,
                               G4_INST *Send1, G4_BB *bb,
                               INST_LIST_ITER InsertBeforePos) {
  G4_Type Ty = FusedSend->getDst()->getType();
  vISA_ASSERT(TypeSize(Ty) == 4, "Unexpected Type!");

  const G4_SendDescRaw *desc = Send0->getMsgDescRaw();
  vISA_ASSERT(desc, "expected raw descriptor");
  if (!desc) {
    return;
  }

  // Use the original option for mov instructions
  G4_ExecSize execSize = Send0->getExecSize();
  int option = Send0->getOption();
  int32_t nMov = desc->ResponseLength();

  // Make sure the response len = 1 for exec_size = 1|2|4
  //
  // Note that the code is designed for exec_size=8. It also
  // works for exec_size=1|2|4 with minor change (keep in mind
  // that nMov = 1 for exec_size=1|2|4)
  vISA_ASSERT((execSize == g4::SIMD8 || nMov == 1),
         "Internal Error(SendFusion) : unexpected message response length!");

  G4_VarBase *Payload = FusedSend->getDst()->getBase();
  G4_VarBase *Dst0 = getVarBase(Send0->getDst()->getBase(), Ty);
  G4_VarBase *Dst1 = getVarBase(Send1->getDst()->getBase(), Ty);
  int16_t Off0 = Send0->getDst()->getRegOff();
  int16_t Off1 = Send1->getDst()->getRegOff();
  const RegionDesc *stride1 = Builder->getRegionStride1();

  G4_SrcRegRegion *S;
  G4_DstRegRegion *D;
  // Copy to Dst0
  for (int i = 0; i < nMov; ++i) {
    S = Builder->createSrc(Payload, 2 * i, 0, stride1, Ty);
    D = Builder->createDst(Dst0, Off0 + i, 0, 1, Ty);
    G4_INST *Inst0 = Builder->createMov(execSize, D, S, option, false);
    bb->insertBefore(InsertBeforePos, Inst0);

    // Update DefUse
    FusedSend->addDefUse(Inst0, Opnd_src0);
    Send0->copyUsesTo(Inst0, true);
  }

  // Copy to Dst1
  for (int i = 0; i < nMov; ++i) {
    S = Builder->createSrc(Payload, (execSize == 8 ? 2 * i + 1 : 2 * i),
                           (execSize == 8) ? 0 : execSize, stride1, Ty);
    D = Builder->createDst(Dst1, Off1 + i, 0, 1, Ty);
    G4_INST *Inst1 = Builder->createMov(execSize, D, S, option, false);
    bb->insertBefore(InsertBeforePos, Inst1);

    // Update DefUse
    FusedSend->addDefUse(Inst1, Opnd_src0);
    Send1->copyUsesTo(Inst1, true);
  }
}

void SendFusion::initDMaskModInfo() {
  for (BB_LIST_ITER BI = CFG->begin(), BE = CFG->end(); BI != BE; ++BI) {
    G4_BB *BB = *BI;
    for (INST_LIST_ITER II = BB->begin(), IE = BB->end(); II != IE; ++II) {
      G4_INST *inst = *II;
      G4_DstRegRegion *dst = inst->getDst();
      // Check if sr0.2 (DW) is modified.
      if (dst && dst->isAreg() && dst->isSrReg() &&
          ((dst->getLeftBound() <= 8 && dst->getRightBound() >= 8) ||
           (dst->getLeftBound() <= 11 && dst->getRightBound() >= 11))) {
        LastSR0ModInstPerBB[BB] = inst;
      }
    }
  }
}

// Dispatch mask
// The real channel mask for a thread is computed as
//          sr0.2 & ce0
// This function saves sr0.2 (DMask) in a variable in the entry BB (one for
// each shader/kernel) for the later use. We have to do (sr0.2 & ce0) for each
// BB as ce0 reflects the channel enable under control flow, and each BB might
// have different value of ce0.
void SendFusion::createDMask(G4_BB *bb, INST_LIST_ITER InsertBeforePos) {
  // (W) mov (1|M0) r10.0<1>:ud sr0.2.0<0;1,0>:ud
  G4_Declare *dmaskDecl = Builder->createTempVar(1, Type_UD, Any, "DMask");
  G4_VarBase *sr0 = Builder->phyregpool.getSr0Reg();
  G4_SrcRegRegion *Src =
      Builder->createSrc(sr0, 0, 2, Builder->getRegionScalar(), Type_UD);
  G4_DstRegRegion *Dst =
      Builder->createDst(dmaskDecl->getRegVar(), 0, 0, 1, Type_UD);
  G4_INST *Inst =
      Builder->createMov(g4::SIMD1, Dst, Src, InstOpt_WriteEnable, false);
  bb->insertBefore(InsertBeforePos, Inst);

  // update DefUse info
  CFG->globalOpndHT.addGlobalOpnd(Dst);

  // Save DMaskUD for use later.
  DMaskUD = dmaskDecl->getRegVar();

  for (auto II = LastSR0ModInstPerBB.begin(), IE = LastSR0ModInstPerBB.end();
       II != IE; ++II) {
    G4_BB *BB = II->first;
    G4_INST *inst = II->second;
    INST_LIST_ITER InsertPos = std::find(BB->begin(), BB->end(), inst);
    ++InsertPos;

    G4_SrcRegRegion *S =
        Builder->createSrc(sr0, 0, 2, Builder->getRegionScalar(), Type_UD);
    G4_DstRegRegion *D =
        Builder->createDst(dmaskDecl->getRegVar(), 0, 0, 1, Type_UD);
    G4_INST *Inst =
        Builder->createMov(g4::SIMD1, D, S, InstOpt_WriteEnable, false);
    BB->insertBefore(InsertPos, Inst);
  }
}

// This function will create a flag for each BB. And this flag is used as pred
// for all fused sends in the BB.  It basically does:
//
//     (W) and (1|M0) r11.0<1>:ud ce0.0<0;1,0>:ud DMaskUD
//     (W) mov (2|M0) r12.0<1>:ub r11.0<0;1,0>:ub
//     (W) mov (1|M0) f0.0<1>:uw  r12.0<1>:uw
//
// where DMaskUD is computed in createDMask. Note that those instructions are
// right before the location of first send fusion, not in the begining of BB
// (as BB might have sr0 modifying instruction before those send instructions).
//
void SendFusion::createFlagPerBB(G4_BB *bb, INST_LIST_ITER InsertBeforePos) {
  // FlagPerBB is saved for use later.
  G4_Declare *flagDecl = Builder->createTempFlag(1, "FlagPerBB");
  G4_VarBase *FlagPerBB = flagDecl->getRegVar();
  const RegionDesc *scalar = Builder->getRegionScalar();

  G4_Declare *tmpDecl = Builder->createTempVar(1, Type_UD, Any, "Flag");
  G4_INST *Inst0;
  if (WAce0Read) {
    // (W) mov (1|M0) WAce0:uw, 0
    // cmp (16|M0) (eq)WAce0 r0:uw r0:uw
    // (W) mov(1|M0) tmpDst1  WAce0:uw
    G4_Declare *flagDecl = Builder->createTempFlag(1, "WAce0");
    G4_RegVar *flagVar = flagDecl->getRegVar();
    G4_DstRegRegion *flag = Builder->createDst(flagVar, 0, 0, 1, Type_UW);

    G4_INST *I0 =
        Builder->createMov(g4::SIMD1, flag, Builder->createImm(0, Type_UW),
                           InstOpt_WriteEnable, false);
    bb->insertBefore(InsertBeforePos, I0);

    G4_SrcRegRegion *r0_0 =
        Builder->createSrc(Builder->getRealR0()->getRegVar(), 0, 0,
                           Builder->getRegionStride1(), Type_UW);
    G4_SrcRegRegion *r0_1 =
        Builder->createSrc(Builder->getRealR0()->getRegVar(), 0, 0,
                           Builder->getRegionStride1(), Type_UW);
    G4_CondMod *flagCM = Builder->createCondMod(Mod_e, flagVar, 0);
    G4_DstRegRegion *nullDst = Builder->createNullDst(Type_UW);
    // Hard-coded simd8 here!
    G4_INST *I1 =
        Builder->createInternalInst(NULL, G4_cmp, flagCM, g4::NOSAT, g4::SIMD8,
                                    nullDst, r0_0, r0_1, InstOpt_M0);
    bb->insertBefore(InsertBeforePos, I1);

    G4_SrcRegRegion *flagSrc =
        Builder->createSrc(flagVar, 0, 0, Builder->getRegionScalar(), Type_UW);
    G4_DstRegRegion *tmpDst1 =
        Builder->createDst(tmpDecl->getRegVar(), 0, 0, 1, Type_UW);
    Inst0 = Builder->createMov(g4::SIMD1, tmpDst1, flagSrc, InstOpt_WriteEnable,
                               false);
    bb->insertBefore(InsertBeforePos, Inst0);

    // update DefUse
    I1->addDefUse(Inst0, Opnd_src0);
    I0->addDefUse(Inst0, Opnd_src0);
  } else {
    // (W) and (1|M0) tmp<1>:ud ce0.0<0;1,0>:ud DMaskUD:ud
    G4_SrcRegRegion *ce0Src = Builder->createSrc(
        Builder->phyregpool.getMask0Reg(), 0, 0, scalar, Type_UD);
    G4_SrcRegRegion *dmaskSrc =
        Builder->createSrc(DMaskUD, 0, 0, scalar, Type_UD);
    G4_DstRegRegion *tmpDst =
        Builder->createDst(tmpDecl->getRegVar(), 0, 0, 1, Type_UD);
    Inst0 = Builder->createBinOp(G4_and, g4::SIMD1, tmpDst, ce0Src, dmaskSrc,
                                 InstOpt_WriteEnable, false);
    bb->insertBefore(InsertBeforePos, Inst0);
  }

  //  Duplicate 8-bit mask to the next 8 bits
  //  (W) mov (2|M0) tmp:ub tmp.0<0;1,0>:ub
  G4_Declare *tmpUBDecl = Builder->createTempVar(4, Type_UB, Any, "Flag");
  tmpUBDecl->setAliasDeclare(tmpDecl, 0);
  G4_SrcRegRegion *S =
      Builder->createSrc(tmpUBDecl->getRegVar(), 0, 0, scalar, Type_UB);
  G4_DstRegRegion *D =
      Builder->createDst(tmpUBDecl->getRegVar(), 0, 0, 1, Type_UB);
  G4_INST *Inst1 =
      Builder->createMov(g4::SIMD2, D, S, InstOpt_WriteEnable, false);
  bb->insertBefore(InsertBeforePos, Inst1);

  // update DefUse
  Inst0->addDefUse(Inst1, Opnd_src0);

  // (W) mov (1|M0) flagPerBB.0<1>:UW tmp.0<1>:UW
  G4_Declare *tmpUW = Builder->createTempVar(1, Type_UW, Any);
  tmpUW->setAliasDeclare(tmpDecl, 0);
  G4_SrcRegRegion *Src =
      Builder->createSrc(tmpUW->getRegVar(), 0, 0, scalar, Type_UW);
  G4_DstRegRegion *flag = Builder->createDst(FlagPerBB, 0, 0, 1, Type_UW);
  FlagDefPerBB =
      Builder->createMov(g4::SIMD1, flag, Src, InstOpt_WriteEnable, false);
  bb->insertBefore(InsertBeforePos, FlagDefPerBB);

  // update DefUse
  Inst1->addDefUse(FlagDefPerBB, Opnd_src0);
}

// Fuse IT0 and IT1, where IT0 precedes IT1 in the same BB.
// IsSink : true  -> IT0 moves to IT1;
//          false -> IT1 moves to IT0
void SendFusion::doFusion(INST_LIST_ITER IT0, INST_LIST_ITER IT1, bool IsSink) {
  // This function does the following:
  //    Given the following two sends:
  //      send (8|M0)  r40  r39  0xA  0x02110802 // byte scattered read,
  //      resLen=1, msgLen=1 send (8|M0)  r42  r41  0xA  0x02110802 // byte
  //      scattered read, resLen=1, msgLen=1
  //    It generates the following:
  //      (1) Setting flag register
  //            (Need to do Sr0.2 & ce0.0 to get real channel mask)
  //         (1.1) Dispatch mask
  //              (W) mov (1|M0) r10.0<1>:ud sr0.2.0<0;1,0>:ud
  //         (1.2)
  //              (W) and (1|M0) r11.0<1>:ud ce0.0<0;1,0>:ud r10.0<0;1,0>:ud
  //              (W) mov (2|M0) r12.0<1>:ub r11.0<0;1,0>:ub
  //              (W) mov (1|M0) f0.0<1>:uw 0:ud r12.0<1>:uw
  //
  //         Note : (1.1) is saved in entry BB (one for each shader). (1.2) is
  //         saved in each BB
  //                (one for each BB as each BB might have different ce0).
  //                Also, if the original sends are WriteEnabled, no Pred is
  //                needed.
  //      (2) Generating send (16)
  //                   mov (8|M0) r50.0<1>:ud r39.0<8;8,1>:ud
  //                   mov (8|M0) r51.0<1>:ud r41.0<8;8,1>:ud
  //          (W&f0.0) send (16|M0) r52:f r50 0xA 0x 0x4210802
  //                   mov (8|M0) r40.0<1>:ud r52.0<8;8,1>:ud
  //                   mov (8|M0) r42.0<1>:ud r53.0<8;8,1>:ud
  //
  //          Note this is to explain the idea. For this case, this function
  //          actually generate split send, which avoid packing payloads before
  //          the split send.

  G4_INST *I0 = *IT0;
  G4_INST *I1 = *IT1;
  INST_LIST_ITER InsertBeforePos = IsSink ? IT1 : IT0;

  // Use I0 as both I0 and I1 have the same properties
  const G4_SendDescRaw *desc = I0->getMsgDescRaw();
  vISA_ASSERT(desc, "expected raw descriptor");
  if (!desc)
    return;

  G4_ExecSize execSize = I0->getExecSize();
  bool isWrtEnable = I0->isWriteEnableInst();
  bool isSplitSend = I0->isSplitSend();

  if (!isWrtEnable) {
    // No need to read DMask if WAceRead is true
    if (!WAce0Read && DMaskUD == nullptr) {
      // First time, let's save dispatch mask in the entry BB and
      // use it for all Channel enable calcuation in the entire shader.
      // Note that if dispatch mask is modified in the shader, the DMaskUD
      // will need to be saved right after each modification.
      G4_BB *entryBB = CFG->getEntryBB();
      INST_LIST_ITER beforePos = entryBB->begin();

      // Skip the label if present (only first inst can be label).
      if (beforePos != entryBB->end() && (*beforePos)->isLabel()) {
        ++beforePos;
      }
      createDMask(entryBB, beforePos);
    }

    if (FlagDefPerBB == nullptr) {
      createFlagPerBB(CurrBB, InsertBeforePos);
    }
  }

  G4_VarBase *FlagPerBB = nullptr;
  uint32_t rspLen = desc->ResponseLength();
  uint32_t msgLen = desc->MessageLength();
  uint32_t extMsgLen = desc->extMessageLength();

  // Message header : all candidates have no header (for now).
  //
  // Also for the message type that we handle, if execSize is 1|2|4,
  //     write:  msgLen+extMsgLen = 2; and
  //     read:   msgLen = 1 && rspLen = 1
  uint32_t newMsgLen = 2 * msgLen;
  uint32_t newRspLen = 2 * rspLen;
  uint32_t newExtMsgLen = 2 * extMsgLen;
  if (execSize < 8) {
    // Re-adjust length, note that msgLen should be > 0
    if (execSize == 4 && desc->isA64Message() && msgLen > 0) {
      // addr : 2 GRF; source : 1 GRF
      newMsgLen = (msgLen == 1 ? 2 : 3);
    } else {
      newMsgLen = msgLen;
    }
    newRspLen = rspLen;
    newExtMsgLen = extMsgLen;
  }

  G4_Predicate *Pred = nullptr;
  if (!isWrtEnable) {
    FlagPerBB = FlagDefPerBB->getDst()->asDstRegRegion()->getBase();
    Pred = Builder->createPredicate(PredState_Plus, FlagPerBB, 0);
  }

  G4_Declare *DstD = nullptr;
  G4_DstRegRegion *Dst;
  if (rspLen > 0) {
    G4_Type DstTy = I0->getDst()->getType();
    if (TypeSize(DstTy) != 4) {
      DstTy = Type_UD;
    }
    DstD = Builder->createTempVar(newRspLen * 8, DstTy, Any, "dst");
    Dst = Builder->createDstRegRegion(DstD, 1);
  } else {
    Dst = Builder->createNullDst(Type_UD);
  }

  // No need to set bti here as we handle the case in which bti is imm only.
  // For that imm bti, the descriptor has already contained bti. Thus, we can
  // safely set bti to nullptr here.
  // G4_Operand* bti = (desc->getSurface() ?
  // Builder->duplicateOperand(desc->getSurface()) : nullptr);
  G4_Operand *bti = nullptr;
  uint32_t newFC =
      (execSize < 8 ? desc->getFuncCtrl() : getFuncCtrlWithSimd16(desc));

  // In general, we have the following
  //       send0       <-- IT0
  //        <O0>       <-- other instruction (no dep with send0/send1)
  //       <DepInst>   <-- ToBeSinked/ToBeHoisted
  //        <O1>       <-- other instruction (no dep with send0/send1)
  //       send1       <-- IT1
  //  which is translated into:
  //
  //    sink:    InsertBeforePos = IT1 (send1)
  //           <O0>
  //           <O1>
  //           <packing>         <-- packPayload()
  //           new_send
  //           <unpacking>       <-- unpackPayload()
  //           <DepInst>         <-- doSink()
  //
  //   hoist:  InsertBeforePos = IT0 (send0)
  //           <DepInst>         <-- doHoist()
  //           <packing>         <-- packPayload()
  //           new_send
  //           <unpacking>       <-- unpackPayload()
  //           <O0>
  //           <O1>

  // Special case of two reads whose payloads can be concatenated using split
  // send.
  if (!isSplitSend && execSize == 8 && rspLen > 0 &&
      (msgLen == 1 || (msgLen == 2 && desc->isA64Message())) &&
      extMsgLen == 0) {
    G4_SendDescRaw *newDesc = Builder->createSendMsgDesc(
        newFC, newRspLen, msgLen, desc->getFuncId(), msgLen,
        desc->getExtFuncCtrl(), desc->getAccess(), bti);

    G4_SrcRegRegion *s0 = I0->getOperand(Opnd_src0)->asSrcRegRegion();
    G4_SrcRegRegion *s1 = I1->getOperand(Opnd_src0)->asSrcRegRegion();
    G4_SrcRegRegion *Src0 = Builder->createSrcRegRegion(*s0);
    G4_SrcRegRegion *Src1 = Builder->createSrcRegRegion(*s1);
    G4_INST *sendInst = Builder->createSplitSendInst(
        Pred, G4_sends, g4::SIMD16, Dst, Src0, Src1,
        Builder->createImm(newDesc->getDesc(), Type_UD), InstOpt_WriteEnable,
        newDesc, nullptr, true);

    if (!IsSink) { // move depInst first if doing hoisting
      doHoist(IT0, IT1, InsertBeforePos);
    }
    CurrBB->insertBefore(InsertBeforePos, sendInst);

    // Update DefUse
    if (Pred) {
      FlagDefPerBB->addDefUse(sendInst, Opnd_pred);
    }
    I0->transferDef(sendInst, Opnd_src0, Opnd_src0);
    I1->transferDef(sendInst, Opnd_src0, Opnd_src1);

    // Unpack the result
    unpackPayload(sendInst, I0, I1, CurrBB, InsertBeforePos);

    if (IsSink) {
      // sink dep instructions
      doSink(IT0, IT1, InsertBeforePos);
    }

    // Delete I0 and I1 and updating defuse info
    I0->removeAllUses();
    I0->removeAllDefs();
    I1->removeAllUses();
    I1->removeAllDefs();
    CurrBB->erase(IT0);
    CurrBB->erase(IT1);
    return;
  }

  G4_SendDescRaw *newDesc = Builder->createSendMsgDesc(
      newFC, newRspLen, newMsgLen, desc->getFuncId(), newExtMsgLen,
      desc->getExtFuncCtrl(), desc->getAccess(), bti);

  // First, create fused send.
  const RegionDesc *region = Builder->getRegionStride1();
  G4_Type P0Ty = I0->getOperand(Opnd_src0)->getType();
  if (TypeSize(P0Ty) != 4) {
    P0Ty = Type_UD;
  }
  G4_Declare *P0 = Builder->createTempVar(newMsgLen * 8, P0Ty, Any, "payload0");
  G4_SrcRegRegion *Src0 = Builder->createSrcRegRegion(P0, region);

  G4_Declare *P1 = nullptr;
  G4_INST *sendInst = nullptr;
  if (isSplitSend) {
    G4_Type P1Ty = I0->getOperand(Opnd_src1)->getType();
    if (TypeSize(P1Ty) != 4) {
      P1Ty = Type_UD;
    }
    P1 = Builder->createTempVar(newExtMsgLen * 8, P1Ty, Any, "payload1");
    G4_SrcRegRegion *Src1 = Builder->createSrcRegRegion(P1, region);
    sendInst = Builder->createSplitSendInst(
        Pred, G4_sends, G4_ExecSize(execSize * 2), Dst, Src0, Src1,
        Builder->createImm(newDesc->getDesc(), Type_UD), InstOpt_WriteEnable,
        newDesc, nullptr, true);
  } else {
    sendInst = Builder->createSendInst(
        Pred, G4_send, G4_ExecSize(execSize * 2), Dst, Src0,
        Builder->createImm(newDesc->getDesc(), Type_UD), InstOpt_WriteEnable,
        newDesc, true);
  }

  if (!IsSink) {
    // Hoisting dep instructions first
    doHoist(IT0, IT1, InsertBeforePos);
  }

  // For messages we handle here, payloads are packing/unpacking
  // in an interleaving way.
  packPayload(sendInst, I0, I1, CurrBB, InsertBeforePos);

  // Update DefUse
  if (Pred) {
    FlagDefPerBB->addDefUse(sendInst, Opnd_pred);
  }

  CurrBB->insertBefore(InsertBeforePos, sendInst);

  if (rspLen > 0) {
    // Unpack the result
    unpackPayload(sendInst, I0, I1, CurrBB, InsertBeforePos);
  }

  if (IsSink) {
    // Sink dep instructions
    doSink(IT0, IT1, InsertBeforePos);
  }

  // Delete I0 and I1 and updating defUse info
  I0->removeAllUses();
  I0->removeAllDefs();
  I1->removeAllUses();
  I1->removeAllDefs();
  CurrBB->erase(IT0);
  CurrBB->erase(IT1);
}

bool SendFusion::run(G4_BB *BB) {
  // Prepare for processing this BB
  CurrBB = BB;
  FlagDefPerBB = nullptr;
  CurrBB->resetLocalIds();

  // Found two candidate sends:
  //    1. next to each (no other sends in between), and
  //    2. both have the same message descriptor.
  INST_LIST_ITER II0 = CurrBB->begin();
  INST_LIST_ITER IE = CurrBB->end();
  while (II0 != IE) {
    // Find out two send instructions (inst0 and inst1) that are next
    // to each other, and check to see if they can be fused into a
    // single one.
    G4_INST *inst0 = *II0;
    if (!simplifyAndCheckCandidate(II0)) {
      ++II0;
      continue;
    }

    G4_INST *inst1 = nullptr;
    INST_LIST_ITER II1 = II0;
    ++II1;
    while (II1 != IE) {
      G4_INST *tmp = *II1;
      if (simplifyAndCheckCandidate(II1)) {
        // possible 2nd send to be fused
        if (tmp->opcode() == inst0->opcode() &&
            tmp->getExecSize() == inst0->getExecSize()) {
          if (canFusion(II0, II1)) {
            // Found
            inst1 = tmp;
          }
        }

        // If found (inst1 != null), exit the inner loop to start fusing;
        // if not found, exit the inner loop and use this one (II1) as
        // the 1st send of possible next pair to start the outer loop again.
        //
        // In both case, don't advance II1.
        break;
      }

      ++II1;
      if (tmp->isSend() || tmp->isOptBarrier()) {
        // Don't try to fusion two sends that are separated
        // by other memory/barrier instructions.
        break;
      }
    }

    if (inst1 == nullptr) {
      // No inst1 found b/w II0 and II1.
      // Start finding the next candidate from II1
      II0 = II1;
      continue;
    }

    // At this point, inst0 and inst1 are the pair that can be fused.
    // Now, check if they can be moved to the same position.
    bool sinkable = canSink(II0, II1);
    bool hoistable = false;
    if (!sinkable || numToBeSinked > 1) {
      hoistable = canHoist(II0, II1);
      if (sinkable && hoistable &&
          numToBeHoisted <
              numToBeSinked) { // Hoisting as it moves less instructions
        sinkable = false;
      }
    }
    if (!sinkable && !hoistable) { // Neither sinkable nor hoistable, looking
                                   // for next candidates.
      II0 = II1;
      continue;
    }

    // Perform fusion (either sink or hoist). It also delete
    // II0 and II1 after fusion. Thus, need to save ++II1
    // before invoking doFusion() for the next iteration.
    INST_LIST_ITER next_II = II1;
    ++next_II;
    doFusion(II0, II1, sinkable);

    changed = true;
    II0 = next_II;
  }

  return changed;
}

//
// The main goal is to do the following for SIMD8 shader:
//
//    [(w)] send(8) + send(8) --> (W&flag) send(16)
//
// Either noMask or not. When no NoMask, send insts with
// execsize=1|2|4 are also supported.
//
bool vISA::doSendFusion(FlowGraph *aCFG, Mem_Manager *aMMgr) {
  // If split send isn't supported, simply skip send fusion
  if (!aCFG->builder->useSends()) {
    return false;
  }
  // SendFusion uses both src0 and src1 for addresses (load), but messages
  // based on LSC does not support that (it requires addresses to be in src0
  // only) and SendFusion does not support it. Just skip.
  if (aCFG->builder->getPlatform() >= Xe_DG2) {
    return false;
  }

  SendFusion sendFusion(aCFG, aMMgr);

  bool change = false;
  for (BB_LIST_ITER BI = aCFG->begin(), BE = aCFG->end(); BI != BE; ++BI) {
    G4_BB *BB = *BI;
    if (sendFusion.run(BB)) {
      change = true;
    }
  }
  return change;
}
