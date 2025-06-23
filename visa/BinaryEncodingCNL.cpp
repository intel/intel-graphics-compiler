/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Assertions.h"
#include "BinaryEncodingCNL.h"
#include "BuildIR.h"
using namespace vISA;

////////////////////////////// DST ////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// SRC ///////////////
////////////////////////////// SRC ///////////////

uint32_t BinaryEncodingBase::getEUOpcode(G4_opcode g4opc) {
  G9HDL::EU_OPCODE euopcode = G9HDL::EU_OPCODE_ILLEGAL;
  switch (g4opc) {
    // 3src - complete
  case G4_bfe:
    euopcode = G9HDL::EU_OPCODE_BFE;
    break;
  case G4_bfi2:
    euopcode = G9HDL::EU_OPCODE_BFI2;
    break;
  case G4_csel:
    euopcode = G9HDL::EU_OPCODE_CSEL;
    break;
  case G4_lrp:
    euopcode = G9HDL::EU_OPCODE_LRP;
    break;
  case G4_mad:
    euopcode = G9HDL::EU_OPCODE_MAD;
    break;
  case G4_madm:
    euopcode = G9HDL::EU_OPCODE_MADM;
    break;

    // 1src - complete
  case G4_bfrev:
    euopcode = G9HDL::EU_OPCODE_BFREV;
    break;
  case G4_cbit:
    euopcode = G9HDL::EU_OPCODE_CBIT;
    break;
  case G4_fbh:
    euopcode = G9HDL::EU_OPCODE_FBH;
    break;
  case G4_fbl:
    euopcode = G9HDL::EU_OPCODE_FBL;
    break;
  case G4_lzd:
    euopcode = G9HDL::EU_OPCODE_LZD;
    break;
  case G4_mov:
    euopcode = G9HDL::EU_OPCODE_MOV;
    break;
  case G4_movi:
    euopcode = G9HDL::EU_OPCODE_MOVI;
    break;
  case G4_not:
    euopcode = G9HDL::EU_OPCODE_NOT;
    break;
  case G4_rndd:
    euopcode = G9HDL::EU_OPCODE_RNDD;
    break;
  case G4_rnde:
    euopcode = G9HDL::EU_OPCODE_RNDE;
    break;
  case G4_rndu:
    euopcode = G9HDL::EU_OPCODE_RNDU;
    break;
  case G4_rndz:
    euopcode = G9HDL::EU_OPCODE_RNDZ;
    break;
  case G4_frc:
    euopcode = G9HDL::EU_OPCODE_FRC;
    break;

    // 2src - complete
  case G4_add:
    euopcode = G9HDL::EU_OPCODE_ADD;
    break;
  case G4_addc:
    euopcode = G9HDL::EU_OPCODE_ADDC;
    break;
  case G4_and:
    euopcode = G9HDL::EU_OPCODE_AND;
    break;
  case G4_asr:
    euopcode = G9HDL::EU_OPCODE_ASR;
    break;
  case G4_avg:
    euopcode = G9HDL::EU_OPCODE_AVG;
    break;
  case G4_bfi1:
    euopcode = G9HDL::EU_OPCODE_BFI1;
    break;
  case G4_cmp:
    euopcode = G9HDL::EU_OPCODE_CMP;
    break;
  case G4_cmpn:
    euopcode = G9HDL::EU_OPCODE_CMPN;
    break;
  case G4_dp2:
    euopcode = G9HDL::EU_OPCODE_DP2;
    break;
  case G4_dp3:
    euopcode = G9HDL::EU_OPCODE_DP3;
    break;
  case G4_dp4:
    euopcode = G9HDL::EU_OPCODE_DP4;
    break;
  case G4_dph:
    euopcode = G9HDL::EU_OPCODE_DPH;
    break;
  case G4_line:
    euopcode = G9HDL::EU_OPCODE_LINE;
    break;
  case G4_mac:
    euopcode = G9HDL::EU_OPCODE_MAC;
    break;
  case G4_mach:
    euopcode = G9HDL::EU_OPCODE_MACH;
    break;
  case G4_mul:
    euopcode = G9HDL::EU_OPCODE_MUL;
    break;
  case G4_or:
    euopcode = G9HDL::EU_OPCODE_OR;
    break;
  case G4_pln:
    euopcode = G9HDL::EU_OPCODE_PLN;
    break;
  case G4_sad2:
    euopcode = G9HDL::EU_OPCODE_SAD2;
    break;
  case G4_sada2:
    euopcode = G9HDL::EU_OPCODE_SADA2;
    break;
  case G4_sel:
    euopcode = G9HDL::EU_OPCODE_SEL;
    break;
  case G4_shl:
    euopcode = G9HDL::EU_OPCODE_SHL;
    break;
  case G4_shr:
    euopcode = G9HDL::EU_OPCODE_SHR;
    break;
  case G4_xor:
    euopcode = G9HDL::EU_OPCODE_XOR;
    break;
  case G4_subb:
    euopcode = G9HDL::EU_OPCODE_SUBB;
    break;

    // send type
  case G4_send:
    euopcode = G9HDL::EU_OPCODE_SEND;
    break;
  case G4_sendc:
    euopcode = G9HDL::EU_OPCODE_SENDC;
    break;
  case G4_sends:
    euopcode = G9HDL::EU_OPCODE_SENDS;
    break;
  case G4_sendsc:
    euopcode = G9HDL::EU_OPCODE_SENDSC;
    break;

    // math type
  case G4_math:
    euopcode = G9HDL::EU_OPCODE_MATH;
    break;

    // control flow
  case G4_brc:
    euopcode = G9HDL::EU_OPCODE_BRC;
    break;
  case G4_brd:
    euopcode = G9HDL::EU_OPCODE_BRD;
    break;
  case G4_break:
    euopcode = G9HDL::EU_OPCODE_BREAK;
    break;
  case G4_call:
    euopcode = G9HDL::EU_OPCODE_CALL;
    break;
    // case G4_calla: euopcode = G9HDL::EU_OPCODE_CALLA; break;
  case G4_cont:
    euopcode = G9HDL::EU_OPCODE_CONT;
    break;
  case G4_else:
    euopcode = G9HDL::EU_OPCODE_ELSE;
    break;
  case G4_endif:
    euopcode = G9HDL::EU_OPCODE_ENDIF;
    break;
  case G4_goto:
    euopcode = G9HDL::EU_OPCODE_GOTO;
    break;
  case G4_halt:
    euopcode = G9HDL::EU_OPCODE_HALT;
    break;
  case G4_if:
    euopcode = G9HDL::EU_OPCODE_IF;
    break;
  case G4_jmpi:
    euopcode = G9HDL::EU_OPCODE_JMPI;
    break;
  case G4_join:
    euopcode = G9HDL::EU_OPCODE_JOIN;
    break;
  case G4_return:
    euopcode = G9HDL::EU_OPCODE_RET;
    break;
  case G4_wait:
    euopcode = G9HDL::EU_OPCODE_WAIT;
    break;
  case G4_while:
    euopcode = G9HDL::EU_OPCODE_WHILE;
    break;

    // misc:
  case G4_nop:
    euopcode = G9HDL::EU_OPCODE_NOP;
    break;
  case G4_illegal:
    euopcode = G9HDL::EU_OPCODE_ILLEGAL;
    break;
  case G4_smov:
    euopcode = G9HDL::EU_OPCODE_SMOV;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("Invalid G4 opcode!");
    break;
  }
  return (uint32_t)euopcode;
}

/// \brief Returns the HDL immediate type for a given source operand
///
static inline int GetOperandSrcHDLImmType(G4_Type srcType) {
  int type = G11HDL::SRCIMMTYPE_UD;

  switch (srcType) {
  case Type_UD:
    type = G11HDL::SRCIMMTYPE_UD;
    break;
  case Type_D:
    type = G11HDL::SRCIMMTYPE_D;
    break;
  case Type_UW:
    type = G11HDL::SRCIMMTYPE_UW;
    break;
  case Type_W:
    type = G11HDL::SRCIMMTYPE_W;
    break;
  case Type_UV:
    type = G11HDL::SRCIMMTYPE_UV;
    break;
  case Type_VF:
    type = G11HDL::SRCIMMTYPE_VF;
    break;
  case Type_V:
    type = G11HDL::SRCIMMTYPE_V;
    break;
  case Type_F:
    type = G11HDL::SRCIMMTYPE_F;
    break;
  case Type_UQ:
    type = G11HDL::SRCIMMTYPE_UQ;
    break;
  case Type_Q:
    type = G11HDL::SRCIMMTYPE_Q;
    break;
  case Type_DF:
    type = G11HDL::SRCIMMTYPE_DF;
    break;
  case Type_HF:
    type = G11HDL::SRCIMMTYPE_HF;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("invalid type");
    break;
  }
  return type;
}

/// \brief Returns the HDL source type for a given source operand
///
static inline int GetOperandSrcHDLType(G4_Type regType) {
  int type = G11HDL::SRCTYPE_UD;

  switch (regType) {
  case Type_UD:
    type = G11HDL::SRCTYPE_UD;
    break;
  case Type_D:
    type = G11HDL::SRCTYPE_D;
    break;
  case Type_UW:
    type = G11HDL::SRCTYPE_UW;
    break;
  case Type_W:
    type = G11HDL::SRCTYPE_W;
    break;
  case Type_UB:
    type = G11HDL::SRCTYPE_UB;
    break;
  case Type_B:
    type = G11HDL::SRCTYPE_B;
    break;
  case Type_DF:
    type = G11HDL::SRCTYPE_DF;
    break;
  case Type_F:
    type = G11HDL::SRCTYPE_F;
    break;
  case Type_UQ:
    type = G11HDL::SRCTYPE_UQ;
    break;
  case Type_Q:
    type = G11HDL::SRCTYPE_Q;
    break;
  case Type_HF:
    type = G11HDL::SRCTYPE_HF;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("invalid type");
    break;
  }

  return type;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// ENCODERS ///////////////

////////////////////////////// HEADER ///////////////

/// \brief (Header) Field encoder for instruction opcode
///
void BinaryEncodingCNL::EncodeOpCode(G4_INST *inst,
                                     G9HDL::EU_INSTRUCTION_HEADER &header) {
  G4_opcode opcode = inst->opcode();
  G9HDL::EU_OPCODE euopcode = getEUOpcode(opcode);
  header.SetOpcode(euopcode);
}

G9HDL::EU_OPCODE BinaryEncodingCNL::getEUOpcode(G4_opcode g4opc) {

  switch (g4opc) {
    // GEN11 specific
  case G4_ror:
    return G9HDL::EU_OPCODE_ROR;
  case G4_rol:
    return G9HDL::EU_OPCODE_ROL;
  case G4_dp4a:
    return G9HDL::EU_OPCODE_DP4A;
  default:
    break;
  }

  return (G9HDL::EU_OPCODE)BinaryEncodingBase::getEUOpcode(g4opc);
}

////////////////////////////// HEADER.CONTROLS ///////////////

/// \brief (Helper) Converts v-isa execution size to HDL exec size enumeration
///
static inline G9HDL::EXECSIZE
GetHDLExecSizeFromVISAExecSize(uint32_t execSize) {
  switch (execSize) {
  case ES_1_CHANNEL:
    return G9HDL::EXECSIZE_1_CHANNEL_SCALAR_OPERATION;
  case ES_2_CHANNELS:
    return G9HDL::EXECSIZE_2_CHANNELS;
  case ES_4_CHANNELS:
    return G9HDL::EXECSIZE_4_CHANNELS;
  case ES_8_CHANNELS:
    return G9HDL::EXECSIZE_8_CHANNELS;
  case ES_16_CHANNELS:
    return G9HDL::EXECSIZE_16_CHANNELS;
  case ES_32_CHANNELS:
    return G9HDL::EXECSIZE_32_CHANNELS;
  }
  // Default:
  return G9HDL::EXECSIZE_1_CHANNEL_SCALAR_OPERATION;
}

//////////////////////////////////////////////////////////////////////////
/// \brief (Header) Field encoder for execution size
///
void BinaryEncodingCNL::EncodeExecSize(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_CONTROLS_A &controls) {
  G9HDL::EXECSIZE exSz;
  exSz = GetHDLExecSizeFromVISAExecSize(GetEncodeExecSize(inst));
  controls.SetExecsize(exSz);
}

//////////////////////////////////////////////////////////////////////////
/// \brief (Header) Field encoder for access mode bit
///
void BinaryEncodingCNL::EncodeAccessMode(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_CONTROLS_A &controls) {
  if (inst->isAligned1Inst()) {
    controls.SetAccessmode(G9HDL::ACCESSMODE_ALIGN1);
  } else if (inst->isAligned16Inst()) {
    controls.SetAccessmode(G9HDL::ACCESSMODE_ALIGN16);
  }
}

//////////////////////////////////////////////////////////////////////////
/// \brief (Header) Field encoder for QTR control fields
///
void BinaryEncodingCNL::EncodeQtrControl(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_CONTROLS_A &controls) {
  G9HDL::QTRCTRL qtrCtrl = G9HDL::QTRCTRL_1Q;
  G9HDL::NIBCTRL nibCtrl = G9HDL::NIBCTRL::NIBCTRL_EVEN;

  switch (inst->getMaskOffset()) {
  case 0:
    qtrCtrl = G9HDL::QTRCTRL_1Q;
    nibCtrl = G9HDL::NIBCTRL_ODD;
    break;
  case 4:
    qtrCtrl = G9HDL::QTRCTRL_1Q;
    nibCtrl = G9HDL::NIBCTRL_EVEN;
    break;
  case 8:
    qtrCtrl = G9HDL::QTRCTRL_2Q;
    nibCtrl = G9HDL::NIBCTRL_ODD;
    break;
  case 12:
    qtrCtrl = G9HDL::QTRCTRL_2Q;
    nibCtrl = G9HDL::NIBCTRL_EVEN;
    break;
  case 16:
    qtrCtrl = G9HDL::QTRCTRL_3Q;
    nibCtrl = G9HDL::NIBCTRL_ODD;
    break;
  case 20:
    qtrCtrl = G9HDL::QTRCTRL_3Q;
    nibCtrl = G9HDL::NIBCTRL_EVEN;
    break;
  case 24:
    qtrCtrl = G9HDL::QTRCTRL_4Q;
    nibCtrl = G9HDL::NIBCTRL_ODD;
    break;
  case 28:
    qtrCtrl = G9HDL::QTRCTRL_4Q;
    nibCtrl = G9HDL::NIBCTRL_EVEN;
    break;
  }

  controls.SetQtrctrl(qtrCtrl);
  controls.SetNibctrl(nibCtrl);
}

/// \brief Field encoder for dependency control check fields
inline void BinaryEncodingCNL::EncodeDepControl(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_CONTROLS_A &controlsA) {
  if (inst->isNoDDChkInst()) {
    if (inst->isNoDDClrInst()) {
      controlsA.SetDepctrl(G9HDL::DEPCTRL_NODDCLR_NODDCHK);
    } else {
      controlsA.SetDepctrl(G9HDL::DEPCTRL_NODDCHK);
    }
  } else {
    if (inst->isNoDDClrInst()) {
      controlsA.SetDepctrl(G9HDL::DEPCTRL_NODDCLR);
    } else {
      controlsA.SetDepctrl(G9HDL::DEPCTRL_NONE);
    }
  }
}

/// \brief Field encoder for thread control field. Includes some logic that is
//         dependant on the platform.
inline void BinaryEncodingCNL::EncodeThreadControl(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_CONTROLS_A &controlsA) {
  if (inst->opcode() == G4_if || inst->opcode() == G4_else ||
      inst->opcode() == G4_endif) {

  }

  else {
    controlsA.SetThreadControl(
        inst->isAtomicInst() ? G9HDL::THREADCTRL_ATOMIC :
                             // CHAI: Add Switch InstOpt support
            (inst->isYieldInst()
                 ? G9HDL::THREADCTRL_SWITCH
                 : (inst->isNoPreemptInst() ? G9HDL::THREADCTRL_NOPREEMPT
                                            : G9HDL::THREADCTRL_NORMAL)));
  }
}

/// \brief Field encoder for ACC write control field
void BinaryEncodingCNL::EncodeAccWrCtrl(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_CONTROLS &instructionControls) {
  if (inst->isAccWrCtrlInst() ||
      (inst->isFlowControl() && inst->opcode() != G4_jmpi &&
       inst->asCFInst()->isBackward())) {
    instructionControls.SetControlsB_Accwrctrl(G9HDL::ACCWRCTRL_UPDATE_ACC);
  }
}

//////////////////////////////////////////////////////////////////////////
//// Field encoder
void BinaryEncodingCNL::EncodeInstModifier(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_CONTROLS &instructionControls) {
  if (inst->getSaturate()) {
    instructionControls.SetControlsB_Saturate(G9HDL::SATURATE_SAT);
  } else {
    instructionControls.SetControlsB_Saturate(
        G9HDL::SATURATE_NO_DESTINATION_MODIFICATION);
  }
}

//////////////////////////////////////////////////////////////////////////
//// Field encoder
void BinaryEncodingCNL::EncodeFlagRegPredicate(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_CONTROLS_A &instructionControlsA) {
  G4_Predicate *pred = inst->getPredicate();
  if (pred) {
    switch (pred->getState()) {
      // plus and undef are normal
    case PredState_Plus:
    case PredState_undef:
      instructionControlsA.SetPredinv(G9HDL::PREDINV_POSITIVE);
      // instructionHeader.SetControl_Predinv(G9HDL::PREDINV_POSITIVE);
      // flagState = PREDICATE_STATE_NORMAL;
      break;
      // minus is invert
    case PredState_Minus:
      instructionControlsA.SetPredinv(G9HDL::PREDINV_NEGATIVE);
      // instructionHeader.SetControl_Predinv(G9HDL::PREDINV_NEGATIVE);
      // flagState = PREDICATE_STATE_INVERT<<4;
      break;
    }

    G9HDL::PREDCTRL predCtrl = G9HDL::PREDCTRL_SEQUENTIAL_FLAG_CHANNEL_MAPPING;

    if (inst->isAligned16Inst()) {
      switch (getAlign16PredCtrl(pred)) {
      case PRED_ALIGN16_DEFAULT:
        predCtrl = G9HDL::PREDCTRL_SEQUENTIAL_FLAG_CHANNEL_MAPPING;
        break;
      case PRED_ALIGN16_X:
        predCtrl = G9HDL::PREDCTRL_REPLICATION_SWIZZLE_X;
        break;
      case PRED_ALIGN16_Y:
        predCtrl = G9HDL::PREDCTRL_REPLICATION_SWIZZLE_Y;
        break;
      case PRED_ALIGN16_Z:
        predCtrl = G9HDL::PREDCTRL_REPLICATION_SWIZZLE_Z;
        break;
      case PRED_ALIGN16_W:
        predCtrl = G9HDL::PREDCTRL_REPLICATION_SWIZZLE_W;
        break;
      case PRED_ALIGN16_ANY4H:
        predCtrl = G9HDL::PREDCTRL_ANY4H;
        break;
      case PRED_ALIGN16_ALL4H:
        predCtrl = G9HDL::PREDCTRL_ALL4H;
        break;
      default:
        vISA_ASSERT_UNREACHABLE("invalid align16 predicate control");
      }
      instructionControlsA.SetPredctrl(predCtrl);
      // instructionHeader.SetControl_Predctrl(predCtrl);
    } else {
      auto pc = pred->getControl();
      if (pc != PRED_DEFAULT)
        predCtrl = (G9HDL::PREDCTRL)GetAlign1PredCtrl(pc);
      instructionControlsA.SetPredctrl(predCtrl);
    }
  }
}

static const unsigned CONDITION_MODIIFER[11] = {
    (unsigned)G9HDL::CONDMODIFIER_Z,  (unsigned)G9HDL::CONDMODIFIER_E,
    (unsigned)G9HDL::CONDMODIFIER_NZ, (unsigned)G9HDL::CONDMODIFIER_NE,
    (unsigned)G9HDL::CONDMODIFIER_G,  (unsigned)G9HDL::CONDMODIFIER_GE,
    (unsigned)G9HDL::CONDMODIFIER_L,  (unsigned)G9HDL::CONDMODIFIER_LE,
    (unsigned)G9HDL::CONDMODIFIER_O, // Mod_o
    (unsigned)G9HDL::CONDMODIFIER_O, // Mod_r
    (unsigned)G9HDL::CONDMODIFIER_U  // Mod_u
};
/// \brief Field encoder
///
void BinaryEncodingCNL::EncodeCondModifier(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_CONTROLS &instructionControls) {
  G4_CondMod *cModifier = inst->getCondMod();
  if (cModifier) {
    G9HDL::CONDMODIFIER value;

    unsigned mod = (unsigned)cModifier->getMod();
    vISA_ASSERT(mod != (unsigned)Mod_r && mod < (unsigned)Mod_cond_undef,
                 // case Mod_r:
                 //     value = G9HDL::CONDMODIFIER_O; //7
                 //     break;
                 "[Verifying]:[ERR]: Invalid conditional modifier:\t");
    value = (G9HDL::CONDMODIFIER)CONDITION_MODIIFER[mod];
    instructionControls.SetCondmodifier(value);
  }
}

/// \brief Encodes instruction header DWORD, common for all types of
/// instructions
inline void
BinaryEncodingCNL::EncodeInstHeader(G4_INST *inst,
                                    G9HDL::EU_INSTRUCTION_HEADER &header) {
  G9HDL::EU_INSTRUCTION_CONTROLS &controls = header.GetControl();
  G9HDL::EU_INSTRUCTION_CONTROLS_A &controlsA = controls.GetControlsA();

  header.Init();
  EncodeOpCode(inst, header);
  EncodeExecSize(inst, controlsA);
  EncodeAccessMode(inst, controlsA);
  EncodeQtrControl(inst, controlsA);
  EncodeThreadControl(inst, controlsA);
  EncodeDepControl(inst, controlsA);
  EncodeFlagRegPredicate(inst, controlsA);
  EncodeAccWrCtrl(inst, controls);
  EncodeInstModifier(inst, controls);
  EncodeCondModifier(inst, controls);

  controls.SetControlsB_Cmptctrl(inst->isCompactedInst()
                                     ? G9HDL::CMPTCTRL_COMPACTED
                                     : G9HDL::CMPTCTRL_NOCOMPACTION);

  if (inst->isBreakPointInst())
    controls.SetControlsB_Debugctrl(G9HDL::DEBUGCTRL_BREAKPOINT);
}

////////////////////////////// DST.OPERAND_CONTROLS ///////////////
////////////////////////////// DST.OPERAND_CONTROLS ///////////////
////////////////////////////// DST.OPERAND_CONTROLS ///////////////
////////////////////////////// DST.OPERAND_CONTROLS ///////////////
////////////////////////////// DST.OPERAND_CONTROLS ///////////////
////////////////////////////// DST.OPERAND_CONTROLS ///////////////
////////////////////////////// DST.OPERAND_CONTROLS ///////////////
////////////////////////////// DST.OPERAND_CONTROLS ///////////////

//////////////////////////////////////////////////////////////////////////
//// Field encoder
inline void BinaryEncodingCNL::EncodeDstHorzStride(
    G4_INST *inst, G4_DstRegRegion *dst,
    G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &opnds) {
  switch (dst->getHorzStride()) {
  case 1:
    // NOTE: not sure if this is correct?
    if (inst->isAligned16Inst()) {
      // NOTE: use align1 union, even if setting align16 field. Masks are equal.
      // note: Although Dst.HorzStride is a don't care for Align16, HW needs
      // this to be programmed as '01'.
      opnds.GetDestinationRegisterRegion_Align1()
          .SetDestinationHorizontalStride(G9HDL::HORZSTRIDE_4_ELEMENTS);
    } else {
      opnds.GetDestinationRegisterRegion_Align1()
          .SetDestinationHorizontalStride(G9HDL::HORZSTRIDE_1_ELEMENTS);
    }
    break;
  case 2:
    opnds.GetDestinationRegisterRegion_Align1().SetDestinationHorizontalStride(
        G9HDL::HORZSTRIDE_2_ELEMENTS);
    break;
  case 4:
    opnds.GetDestinationRegisterRegion_Align1().SetDestinationHorizontalStride(
        G9HDL::HORZSTRIDE_4_ELEMENTS);
    break;
  case UNDEFINED_SHORT:
    opnds.GetDestinationRegisterRegion_Align1().SetDestinationHorizontalStride(
        G9HDL::HORZSTRIDE_1_ELEMENTS);
    break;
  default:
    vISA_ASSERT_UNREACHABLE("wrong dst horizontal stride");
    break;
  }
}

//////////////////////////////////////////////////////////////////////////
//// Field encoder
// NOTE: could change interface, so that 'dst' is passed directly
void BinaryEncodingCNL::EncodeDstChanEn(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &opnds) {
  G4_DstRegRegion *dst = inst->getDst();

  if (dst->isAccRegValid()) {
    // NOTE: this one is special case for instructions that use special
    // accumulators
    opnds.GetDestinationRegisterRegion_Align16().SetDestinationChannelEnable(
        dst->getAccRegSel());
  } else {
    G4_DstRegRegion *dstRegion = static_cast<G4_DstRegRegion *>(dst);

    opnds.GetDestinationRegisterRegion_Align16().SetDestinationChannelEnable(
        getWriteMask(dstRegion));
  }
}

//////////////////////////////////////////////////////////////////////////
//// Field encoder
// NOTE: could change interface, so that 'dst' is passed directly
// NOTE2: Encoding for ARF register type is moved into setting reg destination
// logic.
void BinaryEncodingCNL::EncodeDstRegFile(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &opnds) {
  G4_DstRegRegion *dst = inst->getDst();
  switch (EncodingHelper::GetDstRegFile(
      dst)) { // Bug Line 846, bitrange: 3-4, should be: 35-36
  case REG_FILE_A:
    opnds.SetDestinationRegisterFile(G9HDL::REGFILE_ARF);
    break;
  case REG_FILE_R:
    opnds.SetDestinationRegisterFile(G9HDL::REGFILE_GRF);
    break;
  case REG_FILE_M:
    vISA_ASSERT_UNREACHABLE(0, " Memory is invalid register file on CNL");
    // opnds.SetDestinationRegisterFile(G9HDL::REGFILE_IMM); break;
  default:
    break;
  }
}

//////////////////////////////////////////////////////////////////////////
//// Field encoder
// NOTE: could change interface, so that 'dst' is passed directly
void BinaryEncodingCNL::EncodeDstRegNum(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &opnds) {
  G4_DstRegRegion *dst = inst->getDst();

  if (EncodingHelper::GetDstRegFile(dst) != REG_FILE_A &&
      EncodingHelper::GetDstAddrMode(dst) == ADDR_MODE_IMMED) {
    uint32_t byteAddress = dst->getLinearizedStart();

    if (inst->isAligned1Inst()) {
      // register number: 256 bit (32 byte) aligned part of an address
      // sub-register number: 32 byte address (5 bits encoding) within a GRF
      //  98765 43210
      //  regn |subre

      opnds.GetDestinationRegisterRegion_Align1()
          .SetDestinationRegisterNumber_DestinationRegisterNumber(byteAddress >>
                                                                  5);
      if (dst->isAccRegValid() &&
          kernel.fg.builder->encodeAccRegSelAsAlign1()) {
        vISA_ASSERT((byteAddress & 0x1F) == 0,
                     "subreg must be 0 for dst with special accumulator");
        opnds.GetDestinationRegisterRegion_Align1().SetDestinationSpecialAcc(
            dst->getAccRegSel());
      } else {
        opnds.GetDestinationRegisterRegion_Align1()
            .SetDestinationSubregisterNumber_DestinationSubRegisterNumber(
                byteAddress & 0x1F);
      }
    } else { // align 16
      // register number: 256 bit (32 byte) aligned part of an address
      // sub-register number: first/second 16 byte part of 32 byte address.
      // Encoded with 1 bit.
      //  98765 43210
      //  regn |x0000

      // opnds.GetDestinationRegisterRegion_Align1().
      //     SetDestinationRegisterNumber_DestinationRegisterNumber(byteAddress
      //     >> 5);
      opnds.GetDestinationRegisterRegion_Align16()
          .SetDestinationRegisterNumber_DestinationRegisterNumber(byteAddress >>
                                                                  5);

      // opnds.GetDestinationRegisterRegion_Align1().
      //     SetDestinationSubregisterNumber_DestinationSubRegisterNumber((byteAddress
      //     >> 4) & 0x1);
      opnds.GetDestinationRegisterRegion_Align16()
          .SetDestinationSubregisterNumber((WORD)byteAddress);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//// Field encoder
// NOTE: could change interface, so that 'dst' is passed directly
//
//  SetDstArchRegNum (53:56) + SetDstArchRegFile (57:60)
//  SetDstArchSubRegNumByte (48:52)
//  essentially, this encoding is split in three parts:
//  setting reg num : arch reg file + arch reg number
//  setting sub-reg-num
//  Here, we fuse two original methods into one: EncodeDstArchRegNum and
//  EncodeDstRegFile for ARF
void BinaryEncodingCNL::EncodeDstArchRegNum(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &opnds) {
  G4_DstRegRegion *dst = inst->getDst();

  if (EncodingHelper::GetDstRegFile(dst) == REG_FILE_A &&
      EncodingHelper::GetDstAddrMode(dst) == ADDR_MODE_IMMED) {
    if (EncodingHelper::GetDstArchRegType(dst) != ARCH_REG_FILE_NULL) {
      bool valid;

      unsigned short RegFile =
          (unsigned short)EncodingHelper::GetDstArchRegType(dst); // 4 bits
      unsigned short RegNumValue = dst->ExRegNum(valid);

      // 7654|3210
      // RegF|RegNumVal
      unsigned short EncodedRegNum = RegFile << 4;
      EncodedRegNum = EncodedRegNum | (RegNumValue & 0xF);

      // the same as for align16
      opnds.GetDestinationRegisterRegion_Align1()
          .SetDestinationRegisterNumber_DestinationRegisterNumber(
              EncodedRegNum);
      {
        bool subValid;
        unsigned short RegSubNumValue = dst->ExSubRegNum(subValid);
        unsigned short ElementSizeValue =
            EncodingHelper::GetElementSizeValue(dst);
        uint32_t regOffset = RegSubNumValue * ElementSizeValue;

        if (inst->isAligned1Inst()) {
          // sub-register number: 32 byte address (5 bits encoding) within a GRF
          opnds.GetDestinationRegisterRegion_Align1()
              .SetDestinationSubregisterNumber_DestinationSubRegisterNumber(
                  (WORD)regOffset);
        } else { // align 16
          // sub-register number: first/second 16 byte part of 32 byte address.
          // Encoded with 1 bit.
          //  9876543210
          //  regn|x0000
          // opnds.GetDestinationRegisterRegion_Align16().SetDestinationSubregisterNumber((regOffset
          // >> 4) & 0x1);
          opnds.GetDestinationRegisterRegion_Align16()
              .SetDestinationSubregisterNumber((WORD)regOffset);
        }
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//// Field encoder
// NOTE: could change interface, so that 'dst' is passed directly
void BinaryEncodingCNL::EncodeDstIndirectRegNum(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &opnds) {
  G4_DstRegRegion *dst = inst->getDst();

  if (EncodingHelper::GetDstRegFile(dst) == REG_FILE_R ||
      EncodingHelper::GetDstRegFile(dst) == REG_FILE_M) {
    if (EncodingHelper::GetDstAddrMode(dst) == ADDR_MODE_INDIR) { // Indirect
      bool subValid;
      unsigned short IndAddrRegSubNumValue = 0;
      short IndAddrImmedValue = 0;

      IndAddrRegSubNumValue = dst->ExIndSubRegNum(subValid);
      IndAddrImmedValue = dst->ExIndImmVal();

      // the same is for align16
      opnds.GetDestinationRegisterRegion_Align1()
          .SetDestinationAddressSubregisterNumber_AddressSubregisterNumber(
              IndAddrRegSubNumValue);

      /* Set the indirect address immediate value. */
      if (inst->isAligned1Inst()) {
        // bits [0-8]
        opnds.GetDestinationRegisterRegion_Align1()
            .SetDestinationAddressImmediate(IndAddrImmedValue);
        // bit[9:9]
        opnds.SetDestinationAddressImmediate99(IndAddrImmedValue >> 9);
      } else { // here we are setting align16
        // bits [4-8]
        opnds.GetDestinationRegisterRegion_Align16()
            .SetDestinationAddressImmediate84((IndAddrImmedValue >> 4) & 0x1F);
        // bit[9:9], originally: (IndAddrImmedValue / BYTES_PER_OWORD)  >> 5
        opnds.SetDestinationAddressImmediate99(IndAddrImmedValue >> 9);
      }
    }
  }
}

/// \brief Encodes instruction operand (1st DWORD) of instruction
///
/// src0 reg file and type are not encoded here, even though they belong to
/// dword 1
inline void BinaryEncodingCNL::EncodeOperandDst(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &opnds) {
  G4_DstRegRegion *dst = inst->getDst();

  DstBuilder<G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS>::EncodeFlagReg(inst,
                                                                    opnds);
  DstBuilder<G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS>::EncodeMaskCtrl(inst,
                                                                     opnds);

  if (dst == NULL) {
    return;
  }

  EncodeDstRegFile(inst, opnds);
  DstBuilder<G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS>::EncodeOperandDstType(
      inst, opnds);

  if (inst->isAligned16Inst()) {
    EncodeDstChanEn(inst, opnds);
  }
  // Note: dst doesn't have the vertical stride and width
  EncodeDstRegNum(inst, opnds);
  EncodeDstArchRegNum(inst, opnds);
  EncodeDstIndirectRegNum(inst, opnds);

  EncodeDstHorzStride(inst, dst, opnds);
  DstBuilder<G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS>::EncodeDstAddrMode(inst,
                                                                        opnds);
}

// EU_INSTRUCTION_BASIC_ONE_SRC
//   (EU_INSTRUCTION_SOURCES_REG
//       EU_INSTRUCTION_OPERAND_SRC_REG_ALIGN1
//           or
//       EU_INSTRUCTION_OPERAND_SRC_REG_ALIGN16)
//     or
//   (EU_INSTRUCTION_SOURCES_IMM32

inline void BinaryEncodingCNL::EncodeOneSrcInst(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_BASIC_ONE_SRC &oneSrc) {
  EncodeInstHeader(inst, oneSrc.Common.Header);
  EncodeOperandDst(inst, oneSrc.Common.OperandControls);
  G4_Operand *src0 = inst->getSrc(0);

  // EncodeSrc0RegFile
  oneSrc.GetOperandControls().SetSrc0Regfile(
      TranslateVisaToHDLRegFile(EncodingHelper::GetSrcRegFile(src0)));

  // EncodeSrc0Type
  if (src0->isImm()) {
    oneSrc.GetOperandControls().SetSrc0Srctype_Imm(
        GetOperandSrcHDLImmType(src0->getType()));
  } else {
    oneSrc.GetOperandControls().SetSrc0Srctype(
        GetOperandSrcHDLType(src0->getType()));
  }

  if (src0->isImm()) {
    // this should be compiled as dead code in non-debug mode
    vISA_ASSERT(inst->opcode() == G4_mov || src0->getTypeSize() != 8,
                 "only Mov is allowed for 64-bit immediate");
    if (src0->getTypeSize() == 8) {
      G9HDL::EU_INSTRUCTION_IMM64_SRC *ptr =
          (G9HDL::EU_INSTRUCTION_IMM64_SRC *)&oneSrc;
      EncodeSrcImm64Data(*ptr, src0);
    } else {
      // EncodeSrcImmData(oneSrc.GetImmsource(), src0);
      SrcBuilder<G9HDL::EU_INSTRUCTION_SOURCES_IMM32, 0>::EncodeSrcImmData(
          oneSrc.GetImmsource(), src0);
    }
  } else {
    SrcBuilder<G9HDL::EU_INSTRUCTION_SOURCES_REG,
               0>::EncodeEuInstructionSourcesReg(inst, src0,
                                                 oneSrc.GetRegsource(),
                                                 *this); // by reference
  }
}

inline void BinaryEncodingCNL::EncodeTwoSrcInst(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_BASIC_TWO_SRC &twoSrc) {

  EncodeInstHeader(inst, twoSrc.Common.Header);
  EncodeOperandDst(inst, twoSrc.Common.OperandControls);
  G4_Operand *src0 = inst->getSrc(0);
  G4_Operand *src1 = inst->getSrc(1);

  // EncodeSrc0RegFile
  twoSrc.GetOperandControls().SetSrc0Regfile(
      TranslateVisaToHDLRegFile(EncodingHelper::GetSrcRegFile(src0)));

  [[maybe_unused]]
  bool src0ImmOk = inst->isMath() && inst->asMathInst()->isOneSrcMath();
  vISA_ASSERT(src0ImmOk || !src0->isImm(),
               "src0 is immediate in two src instruction!");
  // EncodeSrc0Type
  if (src0->isImm()) {
    twoSrc.GetOperandControls().SetSrc0Srctype_Imm(
        GetOperandSrcHDLImmType(src0->getType()));
  } else {
    if (inst->isSend()) {
      twoSrc.GetOperandControls().SetSrc0Srctype(GetOperandSrcHDLType(Type_F));
    } else {
      twoSrc.GetOperandControls().SetSrc0Srctype(
          GetOperandSrcHDLType(src0->getType()));
    }
  }

  if (src0->isImm()) {
    vISA_ASSERT(src0->getTypeSize() < 8,
                 "only Mov is allowed for 64bit immediate");
    // FIXME: feels like this should be 0 here, but it gives a type mismatch as
    // the headers assume src0 must be REG
    SrcBuilder<G9HDL::EU_INSTRUCTION_SOURCES_REG_IMM, 1>::EncodeSrcImmData(
        twoSrc.GetImmsource(), src0);
  } else {
    SrcBuilder<G9HDL::EU_INSTRUCTION_SOURCES_REG_REG,
               0>::EncodeEuInstructionSourcesReg(inst, src0,
                                                 twoSrc.GetRegsource(),
                                                 *this); // by reference
  }

  // no need to encode one src math instruction
  if (inst->isMath() && src1->isNullReg() && !src0->isImm()) {
    SrcBuilder<G9HDL::EU_INSTRUCTION_SOURCES_REG_REG,
               1>::EncodeEuInstructionNullSourcesReg(inst, src1,
                                                     twoSrc.GetRegsource());
    return;
  }

  twoSrc.GetRegsource().SetSrc1Regfile(
      TranslateVisaToHDLRegFile(EncodingHelper::GetSrcRegFile(src1)));
  if (src1->isImm()) {
    twoSrc.GetImmsource().SetSrc1Srctype(
        GetOperandSrcHDLImmType(src1->getType()));
  }
  // adding to fix above no need to encode type if src0 is immediate and src1 is
  // null reg
  else if (!(inst->isMath() && src1->isNullReg() && src0->isImm())) {
    twoSrc.GetRegsource().SetSrc1Srctype(GetOperandSrcHDLType(src1->getType()));
  }

  if (src1->isImm()) {
    vISA_ASSERT(inst->opcode() == G4_mov || src1->getTypeSize() != 8,
                 "only Mov is allowed for 64bit immediate");

    SrcBuilder<G9HDL::EU_INSTRUCTION_SOURCES_REG_IMM, 1>::EncodeSrcImmData(
        twoSrc.GetImmsource(), src1);
  } else {
    if (src0->isImm()) {
      // src1 must be null, and don't encode anything so it won't overwrite
      // src0's imm value
      vISA_ASSERT(src1->isNullReg(),
                   "src1 must be null ARF if src0 is immediate");
    } else {
      SrcBuilder<G9HDL::EU_INSTRUCTION_SOURCES_REG_REG,
                 1>::EncodeEuInstructionSourcesReg(inst, src1,
                                                   twoSrc.GetRegsource(),
                                                   *this); // by reference
    }
  }
}

/// \brief Given a two-src mask, apply a patch for send instruction
///
void PatchSend(G4_INST *inst, G9HDL::EU_INSTRUCTION_BASIC_TWO_SRC *twoSrc) {
  vISA_ASSERT(inst->getMsgDescRaw(), "expected raw descriptor");
  uint32_t msgDesc = inst->getMsgDescRaw()->getExtendedDesc();
  EncExtMsgDescriptor emd;
  emd.ulData = msgDesc;

  G9HDL::EU_INSTRUCTION_SEND *sendInstruction =
      (G9HDL::EU_INSTRUCTION_SEND *)twoSrc;
  G9HDL::SFID sfid = (G9HDL::SFID)emd.ExtMsgDescriptor.TargetUnitId;

  sendInstruction->SetSharedFunctionIdSfid(sfid);
  sendInstruction->SetExdesc1111(msgDesc);

  // this is a hack, but:
  // fixme: this is missing in auto-header, currently need to override the acc
  // write field
  if (inst->isNoSrcDepSet()) {
    // equivalent to: mybin->SetBits(bitsNoSrcDepSet_0, bitsNoSrcDepSet_1, 1);
    sendInstruction->SetControlsB_Accwrctrl(G9HDL::ACCWRCTRL_UPDATE_ACC); // 0x1
  }

  G9HDL::EOT eot = (G9HDL::EOT)emd.ExtMsgDescriptor.EndOfThread;
  sendInstruction->GetMessage().SetEot(eot);
}

/// \brief EncodeMathControl
///
void PatchMath(G4_INST *inst, G9HDL::EU_INSTRUCTION_BASIC_TWO_SRC *twoSrc) {
  vISA_ASSERT(inst->isMath(), "PatchMath must be called on math instruction.");

  unsigned int MathControlValue = inst->asMathInst()->getMathCtrl();
  unsigned MathFunction = MathControlValue & 0xf;

  G9HDL::EU_INSTRUCTION_MATH *mathInstruction =
      (G9HDL::EU_INSTRUCTION_MATH *)twoSrc;
  G9HDL::FC mathFunctionControl = (G9HDL::FC)MathFunction;
  mathInstruction->SetFunctionControlFc(mathFunctionControl);

  // fixme: what about partial precision bit?
  // if (!mybin->GetIs3Src())     {
  //     mybin->SetBits(bitsMathFunction_0, bitsMathFunction_1, MathFunction);
  //     mybin->SetBits(bitsMathPartPrec_0, bitsMathPartPrec_1, MathPartPrec);
  // }
}

inline G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::DESTINATION_DATA_TYPE
Get3SrcLimitedType(G4_Type type) {
  switch (type) {
  case Type_F:
    return G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::DESTINATION_DATA_TYPE_F;
  case Type_D:
    return G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::DESTINATION_DATA_TYPE_D;
  case Type_UD:
    return G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::DESTINATION_DATA_TYPE_UD;
  case Type_DF:
    return G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::DESTINATION_DATA_TYPE_DF;
  case Type_HF:
    return G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::DESTINATION_DATA_TYPE_HF;
  default:
    break;
  }
  // default
  return G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::DESTINATION_DATA_TYPE_F;
}

inline static G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::SOURCE_DATA_TYPE
Get3SrcLimitedSrcType(G4_Type type) {
  switch (type) {
  case Type_F:
    return G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::SOURCE_DATA_TYPE_F;
  case Type_D:
    return G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::SOURCE_DATA_TYPE_D;
  case Type_UD:
    return G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::SOURCE_DATA_TYPE_UD;
  case Type_DF:
    return G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::SOURCE_DATA_TYPE_DF;
  case Type_HF:
    return G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::SOURCE_DATA_TYPE_HF;
  default:
    break;
  }
  // default
  return G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC::SOURCE_DATA_TYPE_F;
}

/// Gets align1 ternary instruction limited type, given execution type
/// (integer/float)
inline static G9HDL::TERNARYALIGN1DATATYPE Get3SrcAlign1LimitedSrcType(
    G4_Type type, G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::EXECUTION_DATATYPE
                      isFloatExecutionType) {
  if (isFloatExecutionType ==
      G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::EXECUTION_DATATYPE_FLOAT) {
    switch (type) {
    case Type_F:
      return G9HDL::TERNARYALIGN1DATATYPE::TERNARYALIGN1DATATYPE_F;
    case Type_DF:
      return G9HDL::TERNARYALIGN1DATATYPE::TERNARYALIGN1DATATYPE_DF;
    case Type_HF:
      return G9HDL::TERNARYALIGN1DATATYPE::TERNARYALIGN1DATATYPE_HF;
    case Type_NF:
      return G9HDL::TERNARYALIGN1DATATYPE::TERNARYALIGN1DATATYPE_NF;
    default:
      vISA_ASSERT_UNREACHABLE("wrong type for align1 ternary instruction with float "
                      "execution type.");
      break;
    }

    // Some reasonable default:
    return G9HDL::TERNARYALIGN1DATATYPE::TERNARYALIGN1DATATYPE_F;
  } else {
    switch (type) {
    case Type_UD:
      return G9HDL::TERNARYALIGN1DATATYPE::TERNARYALIGN1DATATYPE_UD;
    case Type_D:
      return G9HDL::TERNARYALIGN1DATATYPE::TERNARYALIGN1DATATYPE_D;
    case Type_UW:
      return G9HDL::TERNARYALIGN1DATATYPE::TERNARYALIGN1DATATYPE_UW;
    case Type_W:
      return G9HDL::TERNARYALIGN1DATATYPE::TERNARYALIGN1DATATYPE_W;
    case Type_UB:
      return G9HDL::TERNARYALIGN1DATATYPE::TERNARYALIGN1DATATYPE_UB;
    case Type_B:
      return G9HDL::TERNARYALIGN1DATATYPE::TERNARYALIGN1DATATYPE_B;
    default:
      vISA_ASSERT_UNREACHABLE("wrong type for align1 ternary instruction with integer "
                      "execution type.");
      break;
    }
    // Some reasonable default:
    return G9HDL::TERNARYALIGN1DATATYPE::TERNARYALIGN1DATATYPE_UD;
  }
}

inline void BinaryEncodingCNL::EncodeThreeSrcInst(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC &threeSrc) {
  G4_Operand *src0 = inst->getSrc(0);
  G4_Operand *src1 = inst->getSrc(1);
  G4_Operand *src2 = inst->getSrc(2);
  G4_Operand *dst = inst->getDst();

  EncodeInstHeader(inst, threeSrc.Common.Header);

  // threeSrc doesn't have OperandControls field, need to implement its dst
  // encoding separately

  DstBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC>::EncodeFlagReg(inst,
                                                                   threeSrc);
  DstBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC>::EncodeMaskCtrl(inst,
                                                                    threeSrc);
  {
    threeSrc.SetDestinationDataType(Get3SrcLimitedType(dst->getType()));
    threeSrc.SetSourceDataType(Get3SrcLimitedSrcType(src0->getType()));
    if (src1->getType() == Type_HF) {
      threeSrc.SetSource1Type(1);
    }
    if (src2->getType() == Type_HF) {
      threeSrc.SetSource2Type(1);
    }

    G4_DstRegRegion *dst = inst->getDst();

    // this is for 'special accumulators', encoded with channel enable..
    if (dst->isAccRegValid()) {
      threeSrc.SetDestinationChannelEnable(dst->getAccRegSel());
      // NOTE: this one is special case for instructions that use special
      // accumulators
    } else {
      G4_DstRegRegion *dstRegion = static_cast<G4_DstRegRegion *>(dst);
      threeSrc.SetDestinationChannelEnable(getWriteMask(dstRegion));
    }

    if (EncodingHelper::GetDstRegFile(dst) != REG_FILE_A &&
        EncodingHelper::GetDstAddrMode(dst) == ADDR_MODE_IMMED) {
      uint32_t byteAddress = dst->getLinearizedStart();

      threeSrc.SetDestinationRegisterNumber_DestinationRegisterNumber(
          byteAddress >> 5);
      // must be DWORD aligned
      // 3 bits for subregnum
      threeSrc.SetDestinationSubregisterNumber((byteAddress >> 2) & 0x7);
      vISA_ASSERT(inst->isAligned16Inst(), "3src only support align16 mode");
    }

    // src0
    {
      // EncodeSrc0RepCtrl
      G4_SrcRegRegion *src0Region = src0->asSrcRegRegion();
      G4_SrcRegRegion *src1Region = src1->asSrcRegRegion();
      G4_SrcRegRegion *src2Region = src2->asSrcRegRegion();

      // source modifiers:
      SrcBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC, 0>::EncodeSrcModifier(
          inst, src0, threeSrc);
      SrcBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC, 1>::EncodeSrcModifier(
          inst, src1, threeSrc);
      SrcBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC, 2>::EncodeSrcModifier(
          inst, src2, threeSrc);

      // rep control:
      SrcBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC,
                 0>::Encode3SrcReplicateControl(&threeSrc, src0Region, *this);
      SrcBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC,
                 1>::Encode3SrcReplicateControl(&threeSrc, src1Region, *this);
      SrcBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC,
                 2>::Encode3SrcReplicateControl(&threeSrc, src2Region, *this);

      // chan select:
      SrcBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC, 0>::EncodeSrcChanSelect(
          &threeSrc, inst, src0, src0Region, *this);
      SrcBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC, 1>::EncodeSrcChanSelect(
          &threeSrc, inst, src1, src1Region, *this);
      SrcBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC, 2>::EncodeSrcChanSelect(
          &threeSrc, inst, src2, src2Region, *this);

      SrcBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC, 0>::EncodeSrcRegNum3Src(
          inst, src0, threeSrc);
      SrcBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC, 1>::EncodeSrcRegNum3Src(
          inst, src1, threeSrc);
      SrcBuilder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC, 2>::EncodeSrcRegNum3Src(
          inst, src2, threeSrc);
      // register
      // sub-register
    }
  }
}

inline void BinaryEncodingCNL::EncodeThreeSrcInstAlign1(
    G4_INST *inst, G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC &threeSrc) {
  G4_Operand *src0 = inst->getSrc(0);
  G4_Operand *src1 = inst->getSrc(1);
  G4_Operand *src2 = inst->getSrc(2);
  G4_DstRegRegion *dst = inst->getDst();

  // Common instruction fields:
  EncodeInstHeader(inst, threeSrc.TheStructure.Common.Header);
  DstBuilder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC>::EncodeFlagReg(inst,
                                                                    threeSrc);
  DstBuilder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC>::EncodeMaskCtrl(inst,
                                                                     threeSrc);

  vISA_ASSERT(
      (IS_TYPE_FLOAT_ALL(src0->getType()) &&
       IS_TYPE_FLOAT_ALL(src1->getType()) &&
       IS_TYPE_FLOAT_ALL(src2->getType()) &&
       IS_TYPE_FLOAT_ALL(dst->getType())) ||
          (IS_TYPE_INT(src0->getType()) && IS_TYPE_INT(src1->getType()) &&
           IS_TYPE_INT(src2->getType()) && IS_TYPE_INT(dst->getType())),
      "No mixed mode in align1 ternary encoding.");

  G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::EXECUTION_DATATYPE execType =
      IS_TYPE_FLOAT_ALL(dst->getType())
          ? G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::EXECUTION_DATATYPE_FLOAT
          : G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::EXECUTION_DATATYPE_INTEGER;

  threeSrc.SetExecutionDatatype(execType);

  // DST REGFILE
  switch (EncodingHelper::GetDstRegFile(dst)) {
  case REG_FILE_R:
    threeSrc.SetDestinationRegisterFile(
        G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::DESTINATION_REGISTER_FILE_GRF);
    break;
  case REG_FILE_A:
    threeSrc.SetDestinationRegisterFile(
        G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::DESTINATION_REGISTER_FILE_ARF);
    break;
  default:
    vISA_ASSERT_UNREACHABLE("Invalid dst register file for for align1 ternary "
                    "instruction (expected grf or arf).");
    break;
  }

  // SRC modifiers:
  if (src0->isSrcRegRegion()) {
    threeSrc.SetSource0Modifier(
        BinaryEncodingCNL::GetSrcHLDMod(src0->asSrcRegRegion()));
  } else {
    threeSrc.SetSource0Modifier(G9HDL::SRCMOD_NO_MODIFICATION);
  }

  if (src1->isSrcRegRegion()) {
    threeSrc.SetSource1Modifier(
        BinaryEncodingCNL::GetSrcHLDMod(src1->asSrcRegRegion()));
  } else {
    threeSrc.SetSource1Modifier(G9HDL::SRCMOD_NO_MODIFICATION);
  }
  if (src2->isSrcRegRegion()) {
    threeSrc.SetSource2Modifier(
        BinaryEncodingCNL::GetSrcHLDMod(src2->asSrcRegRegion()));
  } else {
    threeSrc.SetSource2Modifier(G9HDL::SRCMOD_NO_MODIFICATION);
  }

  // SRC0 regfile
  switch (EncodingHelper::GetSrcRegFile(src0)) {
  case REG_FILE_R:
    threeSrc.SetSource0RegisterFile(
        G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::SOURCE_0_REGISTER_FILE_GRF);
    break;
  case REG_FILE_I:
    threeSrc.SetSource0RegisterFile(
        G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::SOURCE_0_REGISTER_FILE_IMM);
    break;
  case REG_FILE_A:
    vISA_ASSERT(src0->getType() == Type_NF,
                 "Invalid register file for src0 in align1 ternary instruction "
                 "(expected grf or imm).");
    threeSrc.SetSource0RegisterFile(
        G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::SOURCE_0_REGISTER_FILE_ARF);
    break;
  default:
    break;
  }

  // SRC1 regfile
  switch (EncodingHelper::GetSrcRegFile(src1)) {
  case REG_FILE_R:
    threeSrc.SetSource1RegisterFile(
        G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::SOURCE_1_REGISTER_FILE_GRF);
    break;
  case REG_FILE_A:
    threeSrc.SetSource1RegisterFile(
        G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::SOURCE_1_REGISTER_FILE_ARF);
    break;
  default:
    vISA_ASSERT_UNREACHABLE("Invalid register file for src1 in align1 ternary "
                    "instruction(expected grf or arf).");
    break;
  }

  // SRC2 regfile
  switch (EncodingHelper::GetSrcRegFile(src2)) {
  case REG_FILE_R:
    threeSrc.SetSource2RegisterFile(
        G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::SOURCE_2_REGISTER_FILE_GRF);
    break;
  case REG_FILE_I:
    threeSrc.SetSource2RegisterFile(
        G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::SOURCE_2_REGISTER_FILE_IMM);
    break;
  default:
    vISA_ASSERT_UNREACHABLE("Invalid register file for src2 in align1 ternary "
                    "instruction(expected grf or imm).");
    break;
  }

  threeSrc.SetDestinationDatatype(
      Get3SrcAlign1LimitedSrcType(dst->getType(), execType));

  // Dst reg/subreg
  switch (EncodingHelper::GetDstRegFile(dst)) {
  case REG_FILE_R: {
    uint32_t byteAddress = dst->getLinearizedStart();
    if (kernel.fg.builder->encodeAccRegSelAsAlign1() && dst->isAccRegValid()) {
      // special accumulator region (acc2-acc9) is encoded as part of subreg
      vISA_ASSERT((byteAddress & 0x1F) == 0,
                   "subreg must be 0 for dst with special accumulator");
      threeSrc.SetDestinationSpecialAcc(dst->getAccRegSel());
    } else {
      threeSrc.SetDestinationSubregisterNumber(byteAddress & 0x1f);
    }
    threeSrc.SetDestinationRegisterNumber(byteAddress >> 5);
    break;
  }
  case REG_FILE_A: {
    bool valid;
    unsigned short RegFile =
        (unsigned short)EncodingHelper::GetDstArchRegType(dst); // 4 bits
    unsigned short RegNumValue = dst->ExRegNum(valid);

    // 7654|3210
    // RegF|RegNumVal
    unsigned short EncodedRegNum = RegFile << 4;
    EncodedRegNum = EncodedRegNum | (RegNumValue & 0xF);
    threeSrc.SetDestinationRegisterNumber(EncodedRegNum);
    break;
  }
  default:
    vISA_ASSERT_UNREACHABLE("Invalid register file for dst in align1 ternary "
                    "instruction(expected grf or acc).");
    break;
  }

  switch (dst->getHorzStride()) {
  case 1:
    threeSrc.SetDestinationHorizontalStride(
        G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::
            DESTINATION_HORIZONTAL_STRIDE_1_ELEMENT);
    break;
  case 2:
    threeSrc.SetDestinationHorizontalStride(
        G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::
            DESTINATION_HORIZONTAL_STRIDE_2_ELEMENT);
    break;
  case UNDEFINED_SHORT:
    vISA_ASSERT_UNREACHABLE(
        "Dst horizontal stride for align1 ternary instruction is undefined.");
    threeSrc.SetDestinationHorizontalStride(
        G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC::
            DESTINATION_HORIZONTAL_STRIDE_1_ELEMENT);
    break;
  default:
    vISA_ASSERT_UNREACHABLE("Wrong dst horizontal stride for align1 ternary "
                        "instruction (is neither 1 nor 2).");
    break;
  }

  // SRC0 fields
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  threeSrc.SetSource0Datatype(
      Get3SrcAlign1LimitedSrcType(src0->getType(), execType));

  if (src0->isSrcRegRegion()) {
    const RegionDesc *regdesc0 = src0->asSrcRegRegion()->getRegion();
    vISA_ASSERT(regdesc0, "Align1 ternary encoder: src0 region desc for "
                           "ternary instruction is null!");

    // look for <N;N,1> contiguous region
    if (regdesc0->vertStride > 8) {
      // TODO: should we care about this case?
      vISA_ASSERT(
          0, "Align1 3 src instruction with vertStride>8 not supported yet.");
    } else {
      // src0: vstride
      switch (regdesc0->vertStride) {
      case 0:
        SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 0>::
            SetSourceVerticalStride(&threeSrc,
                                    G9HDL::TERNARYALIGN1VERTSTRIDE_0_ELEMENTS);
        break;
      case 2:
        SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 0>::
            SetSourceVerticalStride(&threeSrc,
                                    G9HDL::TERNARYALIGN1VERTSTRIDE_2_ELEMENTS);
        break;
      case 4:
        SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 0>::
            SetSourceVerticalStride(&threeSrc,
                                    G9HDL::TERNARYALIGN1VERTSTRIDE_4_ELEMENTS);
        break;
      case 8:
        SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 0>::
            SetSourceVerticalStride(&threeSrc,
                                    G9HDL::TERNARYALIGN1VERTSTRIDE_8_ELEMENTS);
        break;
      default:
        vISA_ASSERT_UNREACHABLE(
            "wrong vertical stride for ternary align1 instruction at src0!");
        break;
      }

      // src0: hstride
      SrcBuilder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC,
                 0>::EncodeSrcHorzStride(inst, &threeSrc, regdesc0, src0);
    }

    vISA_ASSERT(EncodingHelper::GetSrcRegFile(src0) != REG_FILE_I,
                 "Align1 ternary: src0 register file must not be immediate if "
                 "src region is present. ");
    if (EncodingHelper::GetSrcRegFile(src0) != REG_FILE_A) {
      uint32_t byteAddress = src0->getLinearizedStart();
      threeSrc.SetSource0RegisterNumber_SourceRegisterNumber(byteAddress >> 5);
      if (kernel.fg.builder->encodeAccRegSelAsAlign1() &&
          src0->isAccRegValid()) {
        vISA_ASSERT((byteAddress & 0x1F) == 0,
                     "subreg must be 0 for source with special accumualators");
        threeSrc.SetSource0SpecialAcc(src0->getAccRegSel());
      } else {
        threeSrc.SetSource0SubregisterNumber_SourceSubRegisterNumber(
            byteAddress & 0x1f);
      }
    } else {
      vISA_ASSERT(src0->getType() == Type_NF,
                   "only NF type src0 can be accumulator");
      // encode acc
      bool valid;
      unsigned short RegFile =
          (unsigned short)EncodingHelper::GetSrcArchRegType(src0); // 4 bits
      unsigned short RegNumValue = src0->asSrcRegRegion()->ExRegNum(valid);

      // 7654|3210
      // RegF|RegNumVal
      unsigned short EncodedRegNum = RegFile << 4;
      EncodedRegNum = EncodedRegNum | (RegNumValue & 0xF);
      threeSrc.SetSource0RegisterNumber_SourceRegisterNumber(EncodedRegNum);
    }
  } else {
    vISA_ASSERT(EncodingHelper::GetSrcRegFile(src0) == REG_FILE_I,
                 "Align1 ternary: src0 reg file must be immediate if operand "
                 "is immediate.");

    G4_Imm *isrc = (G4_Imm *)src0->asImm();
    threeSrc.SetSource0ImmediateValue((uint32_t)isrc->getImm());
  }

  // SRC1 fields
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  threeSrc.SetSource1Datatype(
      Get3SrcAlign1LimitedSrcType(src1->getType(), execType));

  vISA_ASSERT(EncodingHelper::GetSrcRegFile(src1) != REG_FILE_I,
               "Align1 ternary:src1 immediate register file not supported by "
               "definition.");
  if (EncodingHelper::GetSrcRegFile(src1) != REG_FILE_A) {
    uint32_t byteAddress = src1->getLinearizedStart();
    threeSrc.SetSource1RegisterNumber_SourceRegisterNumber(byteAddress >> 5);
    if (kernel.fg.builder->encodeAccRegSelAsAlign1() && src1->isAccRegValid()) {
      vISA_ASSERT((byteAddress & 0x1F) == 0,
                   "subreg must be 0 for source with special accumualators");
      threeSrc.SetSource1SpecialAcc(src1->getAccRegSel());
    } else {
      threeSrc.SetSource1SubregisterNumber_SourceSubRegisterNumber(byteAddress &
                                                                   0x1f);
    }
  } else {
    // encode acc
    bool valid;
    unsigned short RegFile =
        (unsigned short)EncodingHelper::GetSrcArchRegType(src1); // 4 bits
    unsigned short RegNumValue = src1->asSrcRegRegion()->ExRegNum(valid);

    // 7654|3210
    // RegF|RegNumVal
    unsigned short EncodedRegNum = RegFile << 4;
    EncodedRegNum = EncodedRegNum | (RegNumValue & 0xF);
    threeSrc.SetSource1RegisterNumber_SourceRegisterNumber(EncodedRegNum);
  }

  const RegionDesc *regdesc1 = src1->asSrcRegRegion()->getRegion();
  vISA_ASSERT(regdesc1, "Align1 ternary encoder: src1 region desc for ternary "
                         "instruction is null!");

  // look for <N;N,1> contiguous region
  if (regdesc1->vertStride > 8) {
    // TODO: should we care about this case?
    vISA_ASSERT(
        0, "align1 3 src instruction with vertStride>8 not supported yet.");
  } else {
    SrcBuilder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 1>::EncodeSrcHorzStride(
        inst, &threeSrc, regdesc1, src1);

    // src1: vstride
    switch (regdesc1->vertStride) {
    case 0:
      SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 1>::
          SetSourceVerticalStride(&threeSrc,
                                  G9HDL::TERNARYALIGN1VERTSTRIDE_0_ELEMENTS);
      break;
    case 2:
      SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 1>::
          SetSourceVerticalStride(&threeSrc,
                                  G9HDL::TERNARYALIGN1VERTSTRIDE_2_ELEMENTS);
      break;
    case 4:
      SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 1>::
          SetSourceVerticalStride(&threeSrc,
                                  G9HDL::TERNARYALIGN1VERTSTRIDE_4_ELEMENTS);
      break;
    case 8:
      SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 1>::
          SetSourceVerticalStride(&threeSrc,
                                  G9HDL::TERNARYALIGN1VERTSTRIDE_8_ELEMENTS);
      break;
    default:
      vISA_ASSERT_UNREACHABLE(
          "wrong vertical stride for ternary align1 instruction at src0!");
      break;
    }
  }

  // SRC2 fields
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  threeSrc.SetSource2Datatype(
      Get3SrcAlign1LimitedSrcType(src2->getType(), execType));

  if (src2->isSrcRegRegion()) {
    const RegionDesc *regdesc2 = src2->asSrcRegRegion()->getRegion();
    vISA_ASSERT(regdesc2, "Align1 ternary instruction: src2 region desc for "
                           "instruction is null!");
    vISA_ASSERT(EncodingHelper::GetSrcRegFile(src2) != REG_FILE_I,
                 "Align1 ternary encoder: src2 reg file is immediate even if "
                 "src region present.");

    // src2 reg/subreg

    if (EncodingHelper::GetSrcRegFile(src2) != REG_FILE_A) {
      uint32_t byteAddress = src2->getLinearizedStart();
      threeSrc.SetSource2RegisterNumber_SourceRegisterNumber(byteAddress >> 5);
      if (kernel.fg.builder->encodeAccRegSelAsAlign1() &&
          src2->isAccRegValid()) {
        vISA_ASSERT((byteAddress & 0x1F) == 0,
                     "subreg must be 0 for source with special accumualators");
        threeSrc.SetSource2SpecialAcc(src2->getAccRegSel());
      } else {
        threeSrc.SetSource2SubregisterNumber_SourceSubRegisterNumber(
            byteAddress & 0x1f);
      }
    }

    switch (regdesc2->horzStride) {
    case 0:
      SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 2>::
          SetSourceHorizontalStride(&threeSrc, G9HDL::HORZSTRIDE_0_ELEMENTS);
      break;
    case 1:
      SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 2>::
          SetSourceHorizontalStride(&threeSrc, G9HDL::HORZSTRIDE_1_ELEMENTS);
      break;
    case 2:
      SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 2>::
          SetSourceHorizontalStride(&threeSrc, G9HDL::HORZSTRIDE_2_ELEMENTS);
      break;
    case 4:
      SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 2>::
          SetSourceHorizontalStride(&threeSrc, G9HDL::HORZSTRIDE_4_ELEMENTS);
      break;
    default:
      vISA_ASSERT_UNREACHABLE("Align1 ternary: src2 has non-encodable horizontal "
                          "stride (accepted: 0,1,2,4).");
      break;
    }
  } else {
    vISA_ASSERT(EncodingHelper::GetSrcRegFile(src2) == REG_FILE_I,
                 "Align1 ternary: src2 reg file must be immediate if operand "
                 "is immediate.");

    G4_Imm *isrc = (G4_Imm *)src2->asImm();
    threeSrc.SetSource2ImmediateValue((uint32_t)isrc->getImm());
  }
}

/// \brief Get compaction control bit from already encoded binary instruction
///
uint32_t BinaryEncodingCNL::GetCompactCtrl(BinInst *mybin) {
  G9HDL::EU_INSTRUCTION_HEADER *ptr =
      (G9HDL::EU_INSTRUCTION_HEADER *)&(mybin->DWords);
  return (uint32_t)ptr->GetControl().GetControlsB_Cmptctrl();
}

/// \brief Set compaction control bit. TODO: how to write to mybin->DWords.
/// below doesn't work
///
void BinaryEncodingCNL::SetCompactCtrl(BinInst *mybin, uint32_t value) {
  G9HDL::EU_INSTRUCTION_HEADER *ptr =
      (G9HDL::EU_INSTRUCTION_HEADER *)&(mybin->DWords);
  ptr->GetControl().SetControlsB_Cmptctrl(G9HDL::CMPTCTRL_COMPACTED);
}

/// \brief Encode JIP and(or) UIP at their respective places
///
/// Comments are left from original binary encoder
/// TODO: move as a class method
void BinaryEncodingCNL::SetBranchOffsets(G4_INST *inst, uint32_t JIP,
                                         uint32_t UIP) {
  BinInst *mybin = getBinInst(inst);
  G4_opcode opc = inst->opcode();

  {
    if (opc == G4_if || opc == G4_break || opc == G4_cont || opc == G4_halt ||
        opc == G4_goto || opc == G4_else) {
      // cast binary as branch two src
      // note: if an instruction has both JIP and UIP, then
      // only src0 type and regfile are being set (higher dword is occupied)

      G9HDL::EU_INSTRUCTION_BRANCH_TWO_SRC *twoSrc =
          (G9HDL::EU_INSTRUCTION_BRANCH_TWO_SRC *)mybin->DWords;

      twoSrc->GetOperandControl().SetSrc0Regfile(G9HDL::REGFILE_IMM);
      twoSrc->GetOperandControl().SetSrc0Srctype_Imm(
          GetOperandSrcHDLImmType(Type_D));
      twoSrc->SetJip(JIP);
      twoSrc->SetUip(UIP);
      // SetBranchJIPUIP(mybin, JIP, UIP);
    } else {
      G9HDL::EU_INSTRUCTION_BRANCH_ONE_SRC *oneSrc =
          (G9HDL::EU_INSTRUCTION_BRANCH_ONE_SRC *)mybin->DWords;

      oneSrc->SetSrc1Regfile(G9HDL::REGFILE_IMM);
      oneSrc->SetSrc1Srctype(GetOperandSrcHDLImmType(Type_D));
      oneSrc->SetJip(JIP);
      // SetBranchJIP(mybin, JIP);
    }
  }
}

/// \brief encodes JIP and(or) UIP offsets
///
/// This is done in this separate method, and not during the main
/// encoding phase, since the real jump offsets are visible only
/// now.
bool BinaryEncodingCNL::EncodeConditionalBranches(G4_INST *inst,
                                                  uint32_t insOffset) {
  std::string jipLabel;
  std::string uipLabel;
  int32_t jipOffset = 0;
  int32_t uipOffset = 0;
  G4_opcode op = inst->opcode();

  // while and case only have JIP for all platforms
  // break, cont and halt have both JIP and UIP for all platforms
  if (op == G4_if || op == G4_else || op == G4_endif || op == G4_while ||
      op == G4_break || op == G4_cont || op == G4_halt || op == G4_goto ||
      op == G4_join) {
    G4_Label *jip = inst->asCFInst()->getJip();
    if (jip) {
      int32_t info = GetLabelInfo(jip);
      if (info == -1) {
        return false;
      }
      jipOffset = info - insOffset;
      jipOffset *= (int32_t)JUMP_INST_COUNT_SIZE;
    } else if (op == G4_while || op == G4_endif || op == G4_join) {
      {
        BinInst *mybin = getBinInst(inst);
        G9HDL::EU_INSTRUCTION_BRANCH_ONE_SRC *oneSrc =
            (G9HDL::EU_INSTRUCTION_BRANCH_ONE_SRC *)mybin->DWords;

        oneSrc->SetSrc1Regfile(G9HDL::REGFILE_IMM);
        oneSrc->SetSrc1Srctype(GetOperandSrcHDLImmType(Type_D));
      }
    }
  }

  // halt has both JIP and UIP on all platforms
  // else has JIP for all platforms; else has UIP for BDW only
  if (op == G4_break || op == G4_cont || op == G4_halt || op == G4_if ||
      op == G4_else || op == G4_goto) {
    G4_Label *uip = inst->asCFInst()->getUip();
    if (uip) {
      int32_t info = GetLabelInfo(uip);
      if (info == -1) {
        return false;
      }
      uipOffset = info - insOffset;
      uipOffset *= (uint32_t)JUMP_INST_COUNT_SIZE;
    }
  }

  if (op == G4_endif && jipOffset == 0) {
    jipOffset = INST_SIZE;
  }

  if (jipOffset != 0 || uipOffset != 0) {
    SetBranchOffsets(inst, jipOffset, uipOffset);
  }

  if (op == G4_jmpi && inst->getSrc(0) && inst->getSrc(0)->isLabel()) {
    // find the label's IP count
    G4_Operand *opnd = inst->getSrc(0);
    BinInst *mybin = getBinInst(inst);
    // Calculate the address offset
    // Label has the same IP count as the following instruction,
    // "break 1" is to the fall through instruction
    int32_t info = GetLabelInfo(opnd->asLabel());
    if (info == -1) {
      return false;
    }
    int32_t jmpOffset = info - insOffset;
    if (GetCompactCtrl(mybin))
      jmpOffset -= 1;
    else
      jmpOffset -= 2;

    jmpOffset *= (int32_t)JUMP_INST_COUNT_SIZE;

    // FIXME: add compaction support
    // if (GetCompactCtrl(mybin))
    //{
    //     SetCmpSrc1RegNum(mybin, jmpOffset & 0xff);          // 63:56
    //     SetCmpSrc1Index(mybin, (jmpOffset >> 8)& 0x1f);
    // }
    // else
    {
      BinInst *mybin = getBinInst(inst);
      G9HDL::EU_INSTRUCTION_BRANCH_ONE_SRC *oneSrc =
          (G9HDL::EU_INSTRUCTION_BRANCH_ONE_SRC *)mybin->DWords;

      oneSrc->SetSrc1Regfile(G9HDL::REGFILE_IMM);
      oneSrc->SetSrc1Srctype(GetOperandSrcHDLImmType(Type_D));
      oneSrc->SetJip((uint32_t)jmpOffset);
    }
  }

  if (op == G4_call && inst->getSrc(0) && inst->getSrc(0)->isLabel()) {
    G4_Operand *opnd = inst->getSrc(0);
    int32_t info = GetLabelInfo(opnd->asLabel());
    if (info == -1) {
      return false;
    }

    int32_t jmpOffset = info - insOffset;

    { jmpOffset *= (int32_t)JUMP_INST_COUNT_SIZE; }

    BinInst *mybin = getBinInst(inst);
    G9HDL::EU_INSTRUCTION_BRANCH_ONE_SRC *oneSrc =
        (G9HDL::EU_INSTRUCTION_BRANCH_ONE_SRC *)mybin->DWords;

    oneSrc->SetSource0_SourceVerticalStride(G9HDL::VERTSTRIDE_2_ELEMENTS);
    oneSrc->SetSource0_SourceWidth(G9HDL::WIDTH_4_ELEMENTS);
    oneSrc->SetSource0_SourceHorizontalStride(G9HDL::HORZSTRIDE_1_ELEMENTS);

    oneSrc->SetSrc1Regfile(G9HDL::REGFILE_IMM);
    oneSrc->SetSrc1Srctype(GetOperandSrcHDLImmType(Type_D));
    oneSrc->SetJip((uint32_t)jmpOffset);

    // TODO: do not forget about compacted variant
  }
  return true;
}

/// \brief initializes auto-header generated structure for split send
/// instruction
///
BinaryEncodingCNL::Status
BinaryEncodingCNL::EncodeSplitSend(G4_INST *inst,
                                   G9HDL::EU_INSTRUCTION_SENDS &sends) {
  Status myStatus = SUCCESS;

  G9HDL::EU_INSTRUCTION_BASIC_TWO_SRC *twoSrcMirror =
      (G9HDL::EU_INSTRUCTION_BASIC_TWO_SRC *)&sends;

  EncodeInstHeader(inst, twoSrcMirror->Common.Header);

  // encode dst part
  // trimmed down EncodeOperandDst
  {
    DstBuilder<G9HDL::EU_INSTRUCTION_SENDS>::EncodeFlagReg(inst, sends);
    DstBuilder<G9HDL::EU_INSTRUCTION_SENDS>::EncodeMaskCtrl(inst, sends);
    DstBuilder<G9HDL::EU_INSTRUCTION_SENDS>::EncodeOperandDstType(inst, sends);
    DstBuilder<G9HDL::EU_INSTRUCTION_SENDS>::EncodeDstAddrMode(inst, sends);

    G4_DstRegRegion *dst = inst->getDst();

    // encode dst reg file
    // note: for sends, we have only one bit available for dst reg file
    {
      switch (EncodingHelper::GetDstRegFile(dst)) {
      case REG_FILE_R:
        sends.SetDestinationRegisterFile(G9HDL::REGFILE_GRF);
        break;
      case REG_FILE_A:
        sends.SetDestinationRegisterFile(G9HDL::REGFILE_ARF);
        break;
      default:
        vISA_ASSERT_UNREACHABLE(" Invalid register file for split-send.");
        break;
      }
    }

    if (EncodingHelper::GetDstAddrMode(dst) == ADDR_MODE_INDIR) {
      // addr subregister
      // addr immediate
      bool subValid;
      uint16_t IndAddrRegSubNumValue = dst->ExIndSubRegNum(subValid);
      int16_t IndAddrImmedValue = dst->ExIndImmVal();

      sends.SetDestinationAddressSubregisterNumber(IndAddrRegSubNumValue);
      sends.SetDestinationAddressImmediate84((IndAddrImmedValue >> 4) & 0x1F);
      sends.SetDestinationAddressImmediateSign9((IndAddrImmedValue >> 9) & 0x1);
    } else {
      if (EncodingHelper::GetDstRegFile(dst) != REG_FILE_A) {
        uint32_t byteAddress = dst->getLinearizedStart();
        vISA_ASSERT(byteAddress % 16 == 0,
                     "dst for sends/sendsc must be oword-aligned");

        sends.SetDestinationRegisterNumber(byteAddress >> 5);
        sends.SetDestinationSubregisterNumber4((byteAddress >> 4) & 0x1);
      }
    }
  }

  // encode src1
  {
    G4_Operand *src1 = inst->getSrc(1);

    // src1 reg file - 1 bit
    switch (EncodingHelper::GetSrcRegFile(src1)) {
    case REG_FILE_R:
      sends.SetSrc1Regfile(G9HDL::REGFILE_GRF);
      break;
    case REG_FILE_A:
      sends.SetSrc1Regfile(G9HDL::REGFILE_ARF);
      break;
    default:
      vISA_ASSERT_UNREACHABLE(" Invalid register file for split-send.");
      break;
    }

    G4_SrcRegRegion *src1Region = src1->asSrcRegRegion();

    SrcBuilder<G9HDL::EU_INSTRUCTION_SENDS, 1>::EncodeSrcAddrMode(&sends, inst,
                                                                  src1);
    if (EncodingHelper::GetSrcAddrMode(src1) == ADDR_MODE_INDIR) {
      bool subValid;
      uint16_t IndAddrRegSubNumValue = src1Region->ExIndSubRegNum(subValid);
      int16_t IndAddrImmedValue = src1Region->ExIndImmVal();

      sends.SetSource1_SourceAddressImmediate84((IndAddrImmedValue >> 4) &
                                                0x1F);
      sends.SetSource1_SourceAddressImmediateSign9((IndAddrImmedValue >> 9) &
                                                   0x1);
      sends.SetSource1_SourceAddressSubregisterNumber_0(IndAddrRegSubNumValue);
    } else {
      uint32_t byteAddress = src1->getLinearizedStart();
      vISA_ASSERT(byteAddress % 32 == 0,
                   "src1 for sends/sendsc must be GRF-aligned");
      sends.SetSource1_SourceRegisterNumber(byteAddress >> 5);
      // mybin->SetBits(bitsSendsSrc1RegNum_0, bitsSendsSrc1RegNum_1,
      // byteAddress >> 5);
    }
  }

  // encode src0
  {
    G4_SrcRegRegion *src0 = inst->getSrc(0)->asSrcRegRegion();
    // note: no regfile for src0

    SrcBuilder<G9HDL::EU_INSTRUCTION_SENDS, 0>::EncodeSrcAddrMode(&sends, inst,
                                                                  src0);

    if (EncodingHelper::GetSrcAddrMode(src0) == ADDR_MODE_INDIR) {
      bool subValid;
      uint16_t IndAddrRegSubNumValue = src0->ExIndSubRegNum(subValid);
      int16_t IndAddrImmedValue = src0->ExIndImmVal();

      sends.SetSource0_SourceAddressImmediate84((IndAddrImmedValue >> 4) &
                                                0x1F);
      sends.SetSource0_SourceAddressImmediateSign9((IndAddrImmedValue >> 9) &
                                                   0x1);
      sends.SetSource0_SourceAddressSubregisterNumber(IndAddrRegSubNumValue);
    } else {
      uint32_t byteAddress = src0->getLinearizedStart();
      vISA_ASSERT(byteAddress % 32 == 0,
                   "src1 for sends/sendsc must be GRF-aligned");
      sends.SetSource0_SourceRegisterNumber(byteAddress >> 5);
      // mybin->SetBits(bitsSendsSrc1RegNum_0, bitsSendsSrc1RegNum_1,
      // byteAddress >> 5);
    }
  }

  // encode src2
  {
    G4_Operand *src2 = inst->getSrc(2);
    if (src2 == NULL) {
      return FAILURE;
    }

    if (src2->isImm()) {
      sends.SetSelreg32desc(0);
      sends.GetMessage().GetDWORD(0) = (uint32_t)src2->asImm()->getInt();
    } else if (src2->isSrcRegRegion() && src2->asSrcRegRegion()->isDirectA0()) {
      sends.SetSelreg32desc(1);
    }
  }

  // Patch SFID and EOT
  {
    vISA_ASSERT(inst->getMsgDescRaw(), "expected raw descriptor");
    uint32_t msgDesc = inst->getMsgDescRaw()->getExtendedDesc();
    EncExtMsgDescriptor emd;
    emd.ulData = msgDesc;

    G9HDL::SFID sfid = (G9HDL::SFID)emd.ExtMsgDescriptor.TargetUnitId;
    sends.SetSharedFunctionIdSfid(sfid);

    G9HDL::EOT eot = (G9HDL::EOT)emd.ExtMsgDescriptor.EndOfThread;
    sends.GetMessage().SetEot(eot);

    // additional extended msg desc to be encoded
    sends.SetExdesc96(msgDesc);
    sends.SetExdesc3116(msgDesc);
    sends.SetExdesc1111(msgDesc);

    // encoding for src3 A0 is done in DoAllEncodingSplitSEND later on
  }

  // this is a hack, but:
  // fixme: this is missing in auto-header, currently need to override the acc
  // write field
  if (inst->isNoSrcDepSet()) {
    // equivalent to: mybin->SetBits(bitsNoSrcDepSet_0, bitsNoSrcDepSet_1, 1);
    sends.SetControlsB_Accwrctrl(G9HDL::ACCWRCTRL_UPDATE_ACC); // 0x1
  }

  return myStatus;
}

/// \brief Do customized encoding of WAIT instruction
///
BinaryEncodingCNL::Status BinaryEncodingCNL::DoAllEncodingWAIT(G4_INST *inst) {
  Status myStatus = SUCCESS;

  G9HDL::EU_INSTRUCTION_BASIC_ONE_SRC oneSrc;

  oneSrc.Init();

  EncodeInstHeader(inst, oneSrc.Common.Header);
  EncodeOperandDst(inst, oneSrc.Common.OperandControls);
  G4_Operand *src0 = inst->getSrc(0);

  // EncodeSrc0RegFile
  oneSrc.GetOperandControls().SetSrc0Regfile(
      TranslateVisaToHDLRegFile(EncodingHelper::GetSrcRegFile(src0)));

  // EncodeSrc0Type
  vISA_ASSERT(!src0->isImm(),
               "src0 must not be immediate in WAIT instruction!");
  {
    oneSrc.GetOperandControls().SetSrc0Srctype(
        GetOperandSrcHDLType(src0->getType()));
  }

  SrcBuilder<G9HDL::EU_INSTRUCTION_SOURCES_REG,
             0>::EncodeEuInstructionSourcesReg(inst, src0,
                                               oneSrc.GetRegsource(),
                                               *this); // by reference

  // Dst patching:

  RegFile regFile = EncodingHelper::GetSrcRegFile(src0);
  vISA_ASSERT(regFile == REG_FILE_A,
               "WAIT instruction source has reg file different than ARF!");
  oneSrc.Common.OperandControls.SetDestinationRegisterFile(
      TranslateVisaToHDLRegFile(regFile));

  if (regFile == REG_FILE_A) {
    bool valid;
    G4_SrcRegRegion *src0Region = src0->asSrcRegRegion();
    unsigned short RegFile =
        (unsigned short)EncodingHelper::GetSrcArchRegType(src0); // 4 bits
    unsigned short RegNumValue = src0Region->ExRegNum(valid);
    unsigned short EncodedRegNum =
        PackArchRegTypeAndArchRegFile(RegFile, RegNumValue);

    oneSrc.Common.OperandControls.GetDestinationRegisterRegion_Align1()
        .SetDestinationRegisterNumber_DestinationRegisterNumber(EncodedRegNum);

    {
      bool subValid;
      unsigned short RegSubNumValue = src0Region->ExSubRegNum(subValid);
      unsigned short ElementSizeValue =
          EncodingHelper::GetElementSizeValue(src0);
      uint32_t regOffset = RegSubNumValue * ElementSizeValue;

      if (inst->isAligned1Inst()) {
        // sub-register number: 32 byte address (5 bits encoding) within a GRF
        oneSrc.Common.OperandControls.GetDestinationRegisterRegion_Align1()
            .SetDestinationSubregisterNumber_DestinationSubRegisterNumber(
                (WORD)regOffset);
      } else { // align 16
        // sub-register number: first/second 16 byte part of 32 byte address.
        // Encoded with 1 bit.
        //  9876543210
        //  regn|x0000
        // opnds.GetDestinationRegisterRegion_Align16().SetDestinationSubregisterNumber((regOffset
        // >> 4) & 0x1);
        oneSrc.Common.OperandControls.GetDestinationRegisterRegion_Align16()
            .SetDestinationSubregisterNumber((WORD)regOffset);
      }
    }
  }

  oneSrc.Common.OperandControls.GetDestinationRegisterRegion_Align1()
      .SetDestinationHorizontalStride(G9HDL::HORZSTRIDE_1_ELEMENTS);

  oneSrc.Common.OperandControls.SetDestinationAddressingMode(
      TranslateVisaToHDLAddrMode(EncodingHelper::GetSrcAddrMode(src0)));

  BinInst *bin = getBinInst(inst);
  bin->DWords[0] = oneSrc.GetDWord(0);
  bin->DWords[1] = oneSrc.GetDWord(1);
  bin->DWords[2] = oneSrc.GetDWord(2);
  bin->DWords[3] = oneSrc.GetDWord(3);

  return myStatus;
}

/// \brief Do encoding of JMPI instruction
///
BinaryEncodingCNL::Status BinaryEncodingCNL::DoAllEncodingJMPI(G4_INST *inst) {
  Status myStatus = SUCCESS;

  G9HDL::EU_INSTRUCTION_BRANCH_ONE_SRC brOneSrc;

  brOneSrc.Init();
  EncodeInstHeader(inst, brOneSrc.GetHeader());

  // BEGIN: OPND CONTROL WORD:
  DstBuilder<G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS>::EncodeFlagReg(
      inst, brOneSrc.GetOperandControl());
  DstBuilder<G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS>::EncodeMaskCtrl(
      inst, brOneSrc.GetOperandControl());

  // hardcode:
  brOneSrc.GetOperandControl().SetDestinationRegisterFile(G9HDL::REGFILE_ARF);
  brOneSrc.GetOperandControl().SetDestinationDataType(G11HDL::DSTTYPE_UD);
  brOneSrc.GetOperandControl().SetDestinationAddressingMode(
      G9HDL::ADDRMODE_DIRECT);

  // FIXME: bxml does not have arch reg file enumerations
  unsigned short RegFile = ARCH_REG_FILE_IP; // 4 bits
  unsigned short RegNumValue = 0;
  // 7654|3210
  // RegF|RegNumVal
  unsigned short EncodedRegNum = RegFile << 4;
  EncodedRegNum = EncodedRegNum | (RegNumValue & 0xF);

  // the same as for align16
  brOneSrc.GetOperandControl()
      .GetDestinationRegisterRegion_Align1()
      .SetDestinationRegisterNumber_DestinationRegisterNumber(EncodedRegNum);
  brOneSrc.GetOperandControl()
      .GetDestinationRegisterRegion_Align1()
      .SetDestinationSubregisterNumber_DestinationSubRegisterNumber(0);

  brOneSrc.GetOperandControl()
      .GetDestinationRegisterRegion_Align1()
      .SetDestinationHorizontalStride(G9HDL::HORZSTRIDE_1_ELEMENTS);

  // src0, but belongs to opndCtl dword
  brOneSrc.GetOperandControl().SetSrc0Regfile(G9HDL::REGFILE_ARF);
  brOneSrc.GetOperandControl().SetSrc0Srctype(GetOperandSrcHDLType(Type_UD));

  // END: OPND CONTROL WORD

  // BEGIN: src0
  if (inst->getSrc(0)) {
    SrcOperandEncoder<G9HDL::EU_INSTRUCTION_BRANCH_ONE_SRC,
                      0>::SetSourceAddressingMode(&brOneSrc,
                                                  G9HDL::ADDRMODE_DIRECT);
    // FIXME: bxml does not have arch reg file enumerations
    unsigned short RegFile = ARCH_REG_FILE_IP; // 4 bits
    unsigned short RegNumValue = 0;
    // 7654|3210
    // RegF|RegNumVal
    unsigned short EncodedRegNum = RegFile << 4;
    EncodedRegNum = EncodedRegNum | (RegNumValue & 0xF);
    SrcOperandEncoder<G9HDL::EU_INSTRUCTION_BRANCH_ONE_SRC,
                      0>::SetSourceRegisterNumber(&brOneSrc, EncodedRegNum);
    SrcOperandEncoder<G9HDL::EU_INSTRUCTION_BRANCH_ONE_SRC,
                      0>::SetSourceSubRegisterNumber(&brOneSrc, 0);

    brOneSrc.SetSource0_SourceWidth(G9HDL::WIDTH_1_ELEMENTS);
    if (inst->getSrc(0)->isLabel()) {
      brOneSrc.SetSource0_SourceVerticalStride(G9HDL::VERTSTRIDE_0_ELEMENTS);
      brOneSrc.SetSource0_SourceHorizontalStride(G9HDL::HORZSTRIDE_0_ELEMENTS);
      brOneSrc.SetSource0_SourceModifier(G9HDL::SRCMOD_NO_MODIFICATION);
    }
  }

  if (inst->getSrc(0) && inst->getSrc(0)->isSrcRegRegion()) {

    G9HDL::EU_INSTRUCTION_BASIC_TWO_SRC *ptr =
        (G9HDL::EU_INSTRUCTION_BASIC_TWO_SRC *)&brOneSrc;

    SrcBuilder<G9HDL::EU_INSTRUCTION_SOURCES_REG_REG,
               1>::EncodeEuInstructionSourcesReg(inst, inst->getSrc(0),
                                                 ptr->GetRegsource(),
                                                 *this); // by reference

    ptr->GetRegsource().SetSrc1Regfile(TranslateVisaToHDLRegFile(
        EncodingHelper::GetSrcRegFile(inst->getSrc(0))));

    if (!inst->getSrc(0)->isImm()) {
      ptr->GetRegsource().SetSrc1Srctype(
          GetOperandSrcHDLType(inst->getSrc(0)->getType()));
    }
  }

  // END: src0

  // BEGIN: src1
  //  The rest is encoded in EncodeConditionalBranches
  // END: src1
  BinInst *bin = getBinInst(inst);
  bin->DWords[0] = brOneSrc.GetDWORD(0);
  bin->DWords[1] = brOneSrc.GetDWORD(1);
  bin->DWords[2] = brOneSrc.GetDWORD(2);
  bin->DWords[3] = brOneSrc.GetDWORD(3);

  return myStatus;
}

BinaryEncodingCNL::Status BinaryEncodingCNL::DoAllEncodingCALL(G4_INST *inst) {
  Status myStatus = SUCCESS;

  BinInst *bin = getBinInst(inst);
  G9HDL::EU_INSTRUCTION_BRANCH_ONE_SRC oneSrc;
  oneSrc.Init();

  EncodeInstHeader(inst, oneSrc.GetHeader());
  EncodeOperandDst(inst, oneSrc.GetOperandControl());

  if (inst->getSrc(0) && !inst->getSrc(0)->isLabel()) {
    // indirect call
    G9HDL::EU_INSTRUCTION_BASIC_TWO_SRC *ptr =
        (G9HDL::EU_INSTRUCTION_BASIC_TWO_SRC *)&oneSrc;

    SrcBuilder<G9HDL::EU_INSTRUCTION_SOURCES_REG_REG,
               1>::EncodeEuInstructionSourcesReg(inst, inst->getSrc(0),
                                                 ptr->GetRegsource(),
                                                 *this); // by reference

    ptr->GetRegsource().SetSrc1Regfile(TranslateVisaToHDLRegFile(
        EncodingHelper::GetSrcRegFile(inst->getSrc(0))));

    if (!inst->getSrc(0)->isImm()) {
      ptr->GetRegsource().SetSrc1Srctype(
          GetOperandSrcHDLType(inst->getSrc(0)->getType()));
    }
  } else {
    // Needed for correctness
    oneSrc.SetSrc1Regfile(G9HDL::REGFILE_IMM);
    oneSrc.SetSrc1Srctype(GetOperandSrcHDLImmType(Type_D));
  }

  bin->DWords[0] = oneSrc.GetDWORD(0);
  bin->DWords[1] = oneSrc.GetDWORD(1);
  bin->DWords[2] = oneSrc.GetDWORD(2);
  bin->DWords[3] = oneSrc.GetDWORD(3);

  return myStatus;
}

/// \brief Do encoding of control flow type instructions
///
BinaryEncodingCNL::Status BinaryEncodingCNL::DoAllEncodingCF(G4_INST *inst) {
  Status myStatus = SUCCESS;
  BinInst *bin = getBinInst(inst);

  G9HDL::EU_INSTRUCTION_BRANCH_TWO_SRC brTwoSrc;
  brTwoSrc.Init();
  // brTwoSrc.Common.OperandControls
  EncodeInstHeader(inst, brTwoSrc.GetHeader());
  // fixme: not sure why we are setting only this field for dst, but keep the
  // binary encoding the same as original one...
  brTwoSrc.GetOperandControl()
      .GetDestinationRegisterRegion_Align1()
      .SetDestinationHorizontalStride(G9HDL::HORZSTRIDE_1_ELEMENTS);
  DstBuilder<G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS>::EncodeFlagReg(
      inst, brTwoSrc.GetOperandControl());
  DstBuilder<G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS>::EncodeMaskCtrl(
      inst, brTwoSrc.GetOperandControl());
  bin->DWords[0] = brTwoSrc.GetDWord(0);
  bin->DWords[1] = brTwoSrc.GetDWord(1);
  bin->DWords[2] = brTwoSrc.GetDWord(2);
  bin->DWords[3] = brTwoSrc.GetDWord(3);
  return myStatus;
}

/// \brief Do encoding of split send instruction
///
BinaryEncodingCNL::Status
BinaryEncodingCNL::DoAllEncodingSplitSEND(G4_INST *inst) {
  Status myStatus = SUCCESS;
  BinInst *bin = getBinInst(inst);

  G9HDL::EU_INSTRUCTION_SENDS sends;
  sends.Init();

  EncodeSplitSend(inst, sends);

  bin->DWords[0] = sends.GetDWORD(0);
  bin->DWords[1] = sends.GetDWORD(1);
  bin->DWords[2] = sends.GetDWORD(2);
  bin->DWords[3] = sends.GetDWORD(3);

  // This is a workaround for BXML defect
  G4_Operand *src3 = inst->getSrc(3);
  // additional extended msg desc to be encoded
  // FIXME: does it apply to regular SKL+ sends too?
  if (src3 && src3->isSrcRegRegion() && src3->asSrcRegRegion()->isDirectA0()) {
    bin->SetBits(bitsSendsSelReg32ExDesc_0, bitsSendsSelReg32ExDesc_1, 1);
    bin->SetBits(bitsSendsExDescRegNum_0, bitsSendsExDescRegNum_1,
                 src3->asSrcRegRegion()->getBase()->asRegVar()->getPhyRegOff());
  }

  return myStatus;
}

/// \brief Do encoding of all 'regular' one, two or three-src instructions
///        Treats send and math instructions as two-src and patches them
///        accordingly.
BinaryEncodingCNL::Status
BinaryEncodingCNL::DoAllEncodingRegular(G4_INST *inst) {
  Status myStatus = SUCCESS;

  vISA_ASSERT(!inst->isSplitSend(), "Improper instruction type called with "
                                     "DoAllEncodingRegular: sends or sendsc");

  BinInst *bin = getBinInst(inst);
  int i = inst->getNumSrc();
  switch (i) {
  case 0: {
    // for nop, we have to encode the opcode, borrow the oneSrc format
    G9HDL::EU_INSTRUCTION_BASIC_ONE_SRC oneSrc;
    oneSrc.Init();
    EncodeOpCode(inst, oneSrc.Common.Header);
    bin->DWords[0] = oneSrc.GetDWord(0);
    bin->DWords[1] = oneSrc.GetDWord(1);
    bin->DWords[2] = oneSrc.GetDWord(2);
    bin->DWords[3] = oneSrc.GetDWord(3);
    break;
  }
  case 1:
    G9HDL::EU_INSTRUCTION_BASIC_ONE_SRC oneSrc;
    oneSrc.Init();
    EncodeOneSrcInst(inst, oneSrc);
    bin->DWords[0] = oneSrc.GetDWord(0);
    bin->DWords[1] = oneSrc.GetDWord(1);
    bin->DWords[2] = oneSrc.GetDWord(2);
    bin->DWords[3] = oneSrc.GetDWord(3);
    break;
  case 2:
    G9HDL::EU_INSTRUCTION_BASIC_TWO_SRC twoSrc;
    twoSrc.Init();
    EncodeTwoSrcInst(inst, twoSrc);
    if (inst->isSend()) {
      PatchSend(inst, &twoSrc);
    } else if (inst->isMath()) {
      // fixme: math is only for two-src encoding?
      PatchMath(inst, &twoSrc);
    }

    bin->DWords[0] = twoSrc.GetDWord(0);
    bin->DWords[1] = twoSrc.GetDWord(1);
    bin->DWords[2] = twoSrc.GetDWord(2);
    bin->DWords[3] = twoSrc.GetDWord(3);
    break;
  case 3: {
    if (inst->isAligned1Inst()) {
      G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC threeSrcAlign1;
      threeSrcAlign1.Init();
      EncodeThreeSrcInstAlign1(inst, threeSrcAlign1);
      bin->DWords[0] = threeSrcAlign1.GetRawData(0);
      bin->DWords[1] = threeSrcAlign1.GetRawData(1);
      bin->DWords[2] = threeSrcAlign1.GetRawData(2);
      bin->DWords[3] = threeSrcAlign1.GetRawData(3);

    } else {
      G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC threeSrc;
      threeSrc.Init();
      EncodeThreeSrcInst(inst, threeSrc);
      bin->DWords[0] = threeSrc.GetDWORD(0);
      bin->DWords[1] = threeSrc.GetDWORD(1);
      bin->DWords[2] = threeSrc.GetDWORD(2);
      bin->DWords[3] = threeSrc.GetDWORD(3);
    }

  } break;
  default:
    break;
  }

  return myStatus;
}

/// \brief Do all encoding for an instruction.
BinaryEncodingCNL::Status BinaryEncodingCNL::DoAllEncoding(G4_INST *inst) {
  Status myStatus = SUCCESS;
  bool isFCCall = false, isFCRet = false;

  if (inst->opcode() == G4_label)
    return myStatus;

  if (inst->opcode() == G4_illegal)
    return FAILURE;

  EncodingHelper::mark3Src(inst, *this);

  {

    // Prolog:
    if (inst->opcode() == G4_pseudo_fc_call) {
      inst->asCFInst()->pseudoCallToCall();
      isFCCall = true;
    }

    if (inst->opcode() == G4_pseudo_fc_ret) {
      inst->asCFInst()->pseudoRetToRet();
      isFCRet = true;
    }

    if (inst->opcode() == G4_jmpi) {
      DoAllEncodingJMPI(inst);
    } else if (inst->opcode() == G4_wait) {
      DoAllEncodingWAIT(inst);
    } else if (inst->opcode() == G4_if || inst->opcode() == G4_endif ||
               inst->opcode() == G4_else || inst->opcode() == G4_while ||
               inst->opcode() == G4_break || inst->opcode() == G4_cont ||
               inst->opcode() == G4_halt || inst->opcode() == G4_goto ||
               inst->opcode() == G4_join) {
      DoAllEncodingCF(inst);
    } else if (inst->opcode() == G4_call) {
      DoAllEncodingCALL(inst);
    } else if (inst->isSplitSend()) {
      DoAllEncodingSplitSEND(inst);
    } else if (!EncodingHelper::hasLabelString(inst)) {
      DoAllEncodingRegular(inst);
    }

    // Epilog:
    if (isFCCall == true) {
      inst->setOpcode(G4_pseudo_fc_call);
    }

    if (isFCRet == true) {
      inst->setOpcode(G4_pseudo_fc_ret);
    }
  }

  return myStatus;
}

/// \brief Entry point.
///
/// This is counterpart of ProduceBinaryInstructions from 'old' encoder
void BinaryEncodingCNL::DoAll() {
  std::vector<ForwardJmpOffset> offsetVector;
  FixInst();
  BinaryEncodingBase::InitPlatform();
  // BDW/CHV/SKL/BXT/CNL use the same compaction tables except from 3src.
  for (uint8_t i = 0; i < (int)COMPACT_TABLE_SIZE; i++) {
    BDWCompactControlTable.AddIndex(IVBCompactControlTable[i], i);
    BDWCompactSourceTable.AddIndex(IVBCompactSourceTable[i], i);
    BDWCompactSubRegTable.AddIndex(IVBCompactSubRegTable[i], i);
    BDWCompactSubRegTable.AddIndex1(IVBCompactSubRegTable[i] & 0x1F, i);
    BDWCompactSubRegTable.AddIndex2(IVBCompactSubRegTable[i] & 0x3FF, i);
    if (kernel.getPlatform() >= GENX_ICLLP) {
      BDWCompactDataTypeTableStr.AddIndex(ICLCompactDataTypeTable[i], i);
    } else {
      BDWCompactDataTypeTableStr.AddIndex(BDWCompactDataTypeTable[i], i);
    }
  }

  int globalInstNum = 0;
  int globalHalfInstNum = 0;

  BB_LIST_ITER ib, bend(kernel.fg.end());
  for (ib = kernel.fg.begin(); ib != bend; ++ib) {
    G4_BB *bb = *ib;
    int localInstNum = 0;
    int localHalfInstNum = 0;

    /**
     * Traverse the instruction lists
     */
    INST_LIST_ITER ii, iend(bb->end());
    for (ii = bb->begin(); ii != iend; ++ii) {
      /* do detailed encoding here */
      G4_INST *inst = *ii;
      G4_opcode opcode = inst->opcode();

      if (opcode != G4_label) {
        // reuse "BinInst" from BinaryEncoding.h which can be simplified
        BinInst *bin = new (mem) BinInst();
        setBinInst(inst, bin);

        bin->DWords[0] = 0;
        bin->DWords[1] = 0;
        bin->DWords[2] = 0;
        bin->DWords[3] = 0;

        DoAllEncoding(inst);

        if (inst->opcode() == G4_pseudo_fc_call ||
            inst->opcode() == G4_pseudo_fc_ret) {
          getBinInst(inst)->SetDontCompactFlag(true);
        }

        if (doCompaction()) {
          getBinInst(inst)->SetMustCompactFlag(false);
          getBinInst(inst)->SetDontCompactFlag(inst->isNoCompactedInst());

          /**
           * handling switch/case for gen6: jump table should not be compacted
           */
          bool compacted;
          {
            TIME_SCOPE(ENCODE_COMPACTION);
            compacted = BinaryEncodingBase::compactOneInstruction(inst);
          }

          if (compacted) {
            inst->setCompacted();
          }
        }
        binInstList.push_back(getBinInst(inst));

        if (inst->opcode() >= G4_jmpi && inst->opcode() <= G4_join) {
          if (!EncodeConditionalBranches(inst, globalHalfInstNum)) {
            offsetVector.push_back(ForwardJmpOffset(inst, globalHalfInstNum));
          }
        }
      }

      BuildLabelMap(inst, localHalfInstNum, localInstNum, globalHalfInstNum,
                    globalInstNum);
    } // for inst
  }   // for bb

  kernel.setAsmCount(globalInstNum);
  SetInstCounts((uint32_t)globalHalfInstNum);

  for (auto x = offsetVector.begin(), vEnd = offsetVector.end(); x != vEnd;
       x++) {
    if (!EncodeConditionalBranches(x->inst, x->offset)) {
      vISA_ASSERT(false, "invalid label!");
    }
  }
}
