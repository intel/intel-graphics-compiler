/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_GED_TO_IGA_TRANSLATION_H_
#define _IGA_GED_TO_IGA_TRANSLATION_H_

#include "../../IR/Types.hpp"
#include "ged.h"

namespace iga {
static inline Op translate(GED_OPCODE gedOpcode) {
  Op opcode;
  switch (gedOpcode) {
  case GED_OPCODE_INVALID:
    opcode = Op::ILLEGAL;
    break;
  case GED_OPCODE_add:
    opcode = Op::ADD;
    break;
  case GED_OPCODE_addc:
    opcode = Op::ADDC;
    break;
  case GED_OPCODE_and:
    opcode = Op::AND;
    break;
  case GED_OPCODE_asr:
    opcode = Op::ASR;
    break;
  case GED_OPCODE_avg:
    opcode = Op::AVG;
    break;
  case GED_OPCODE_bfe:
    opcode = Op::BFE;
    break;
  case GED_OPCODE_bfi1:
    opcode = Op::BFI1;
    break;
  case GED_OPCODE_bfi2:
    opcode = Op::BFI2;
    break;
  case GED_OPCODE_bfrev:
    opcode = Op::BFREV;
    break;
  case GED_OPCODE_brc:
    opcode = Op::BRC;
    break;
  case GED_OPCODE_brd:
    opcode = Op::BRD;
    break;
  case GED_OPCODE_break:
    opcode = Op::BREAK;
    break;
  case GED_OPCODE_call:
    opcode = Op::CALL;
    break;
  case GED_OPCODE_calla:
    opcode = Op::CALLA;
    break;
  case GED_OPCODE_cbit:
    opcode = Op::CBIT;
    break;
  case GED_OPCODE_cmp:
    opcode = Op::CMP;
    break;
  case GED_OPCODE_cmpn:
    opcode = Op::CMPN;
    break;
  case GED_OPCODE_cont:
    opcode = Op::CONT;
    break;
  case GED_OPCODE_csel:
    opcode = Op::CSEL;
    break;
  case GED_OPCODE_dim:
    opcode = Op::DIM;
    break;
  case GED_OPCODE_dp2:
    opcode = Op::DP2;
    break;
  case GED_OPCODE_dp3:
    opcode = Op::DP3;
    break;
  case GED_OPCODE_dp4:
    opcode = Op::DP4;
    break;
  case GED_OPCODE_dp4a:
    opcode = Op::DP4A;
    break;
  case GED_OPCODE_dph:
    opcode = Op::DPH;
    break;
  case GED_OPCODE_else:
    opcode = Op::ELSE;
    break;
  case GED_OPCODE_endif:
    opcode = Op::ENDIF;
    break;
  case GED_OPCODE_f16to32:
    opcode = Op::F16TO32;
    break;
  case GED_OPCODE_f32to16:
    opcode = Op::F32TO16;
    break;
  case GED_OPCODE_fbh:
    opcode = Op::FBH;
    break;
  case GED_OPCODE_fbl:
    opcode = Op::FBL;
    break;
  case GED_OPCODE_frc:
    opcode = Op::FRC;
    break;
  case GED_OPCODE_goto:
    opcode = Op::GOTO;
    break;
  case GED_OPCODE_halt:
    opcode = Op::HALT;
    break;
  case GED_OPCODE_if:
    opcode = Op::IF;
    break;
  case GED_OPCODE_illegal:
    opcode = Op::ILLEGAL;
    break;
  case GED_OPCODE_jmpi:
    opcode = Op::JMPI;
    break;
  case GED_OPCODE_join:
    opcode = Op::JOIN;
    break;
  case GED_OPCODE_line:
    opcode = Op::LINE;
    break;
  case GED_OPCODE_lrp:
    opcode = Op::LRP;
    break;
  case GED_OPCODE_lzd:
    opcode = Op::LZD;
    break;
  case GED_OPCODE_mac:
    opcode = Op::MAC;
    break;
  case GED_OPCODE_mach:
    opcode = Op::MACH;
    break;
  case GED_OPCODE_mad:
    opcode = Op::MAD;
    break;
  case GED_OPCODE_madm:
    opcode = Op::MADM;
    break;
  case GED_OPCODE_math:
    opcode = Op::MATH;
    break;
  case GED_OPCODE_mov:
    opcode = Op::MOV;
    break;
  case GED_OPCODE_movi:
    opcode = Op::MOVI;
    break;
  case GED_OPCODE_mul:
    opcode = Op::MUL;
    break;
  case GED_OPCODE_nop:
    opcode = Op::NOP;
    break;
  case GED_OPCODE_not:
    opcode = Op::NOT;
    break;
  case GED_OPCODE_or:
    opcode = Op::OR;
    break;
  case GED_OPCODE_pln:
    opcode = Op::PLN;
    break;
  case GED_OPCODE_ret:
    opcode = Op::RET;
    break;
  case GED_OPCODE_rndd:
    opcode = Op::RNDD;
    break;
  case GED_OPCODE_rnde:
    opcode = Op::RNDE;
    break;
  case GED_OPCODE_rndu:
    opcode = Op::RNDU;
    break;
  case GED_OPCODE_rndz:
    opcode = Op::RNDZ;
    break;
  case GED_OPCODE_rol:
    opcode = Op::ROL;
    break;
  case GED_OPCODE_ror:
    opcode = Op::ROR;
    break;
  case GED_OPCODE_sad2:
    opcode = Op::SAD2;
    break;
  case GED_OPCODE_sada2:
    opcode = Op::SADA2;
    break;
  case GED_OPCODE_sel:
    opcode = Op::SEL;
    break;
  case GED_OPCODE_send:
    opcode = Op::SEND;
    break;
  case GED_OPCODE_sendc:
    opcode = Op::SENDC;
    break;
  case GED_OPCODE_sends:
    opcode = Op::SENDS;
    break;
  case GED_OPCODE_sendsc:
    opcode = Op::SENDSC;
    break;
  case GED_OPCODE_shl:
    opcode = Op::SHL;
    break;
  case GED_OPCODE_shr:
    opcode = Op::SHR;
    break;
  case GED_OPCODE_smov:
    opcode = Op::SMOV;
    break;
  case GED_OPCODE_subb:
    opcode = Op::SUBB;
    break;
  case GED_OPCODE_sync:
    opcode = Op::SYNC;
    break;
  case GED_OPCODE_wait:
    opcode = Op::WAIT;
    break;
  case GED_OPCODE_while:
    opcode = Op::WHILE;
    break;
  case GED_OPCODE_xor:
    opcode = Op::XOR;
    break;
  case GED_OPCODE_add3:
    opcode = Op::ADD3;
    break;
  case GED_OPCODE_bfn:
    opcode = Op::BFN;
    break;
  case GED_OPCODE_dpas:
    opcode = Op::DPAS;
    break;
  case GED_OPCODE_dpasw:
    opcode = Op::DPASW;
    break;
  case GED_OPCODE_macl:
    opcode = Op::MACL;
    break;
  case GED_OPCODE_srnd:
    opcode = Op::SRND;
    break;
  default:
    opcode = Op::ILLEGAL;
    break;
  }

  return opcode;
}

static inline Type translate(GED_PRECISION p) {
  switch (p) {
  case GED_PRECISION_u1:
    return Type::U1;
  case GED_PRECISION_u2:
    return Type::U2;
  case GED_PRECISION_u4:
    return Type::U4;
  case GED_PRECISION_u8:
    return Type::UB;
  case GED_PRECISION_s2:
    return Type::S2;
  case GED_PRECISION_s4:
    return Type::S4;
  case GED_PRECISION_s8:
    return Type::B;
  case GED_PRECISION_f16:
    return Type::HF;
  case GED_PRECISION_bf16:
    return Type::BF;
  case GED_PRECISION_bf8:
    return Type::BF8;
  case GED_PRECISION_tf32:
    return Type::TF32;
  case GED_PRECISION_hf8:
    return Type::HF8;
  default:
    return Type::INVALID;
  }
}


static inline PredCtrl translate(GED_PRED_CTRL pred) {
  PredCtrl predCtrl;

  switch (pred) {
  case GED_PRED_CTRL_Normal:
    predCtrl = PredCtrl::NONE;
    break;
  case GED_PRED_CTRL_Sequential:
    predCtrl = PredCtrl::SEQ;
    break;
  case GED_PRED_CTRL_anyv:
    predCtrl = PredCtrl::ANYV;
    break;
  case GED_PRED_CTRL_allv:
    predCtrl = PredCtrl::ALLV;
    break;
  case GED_PRED_CTRL_any2h:
    predCtrl = PredCtrl::ANY2H;
    break;
  case GED_PRED_CTRL_all2h:
    predCtrl = PredCtrl::ALL2H;
    break;
  case GED_PRED_CTRL_any4h:
    predCtrl = PredCtrl::ANY4H;
    break;
  case GED_PRED_CTRL_all4h:
    predCtrl = PredCtrl::ALL4H;
    break;
  case GED_PRED_CTRL_any8h:
    predCtrl = PredCtrl::ANY8H;
    break;
  case GED_PRED_CTRL_all8h:
    predCtrl = PredCtrl::ALL8H;
    break;
  case GED_PRED_CTRL_any16h:
    predCtrl = PredCtrl::ANY16H;
    break;
  case GED_PRED_CTRL_all16h:
    predCtrl = PredCtrl::ALL16H;
    break;
  case GED_PRED_CTRL_any32h:
    predCtrl = PredCtrl::ANY32H;
    break;
  case GED_PRED_CTRL_all32h:
    predCtrl = PredCtrl::ALL32H;
    break;
  case GED_PRED_CTRL_any:
    predCtrl = PredCtrl::ANY;
    break;
  case GED_PRED_CTRL_all:
    predCtrl = PredCtrl::ALL;
    break;
  case GED_PRED_CTRL_INVALID:
  default:
    predCtrl = PredCtrl::NONE;
    break;
  }

  return predCtrl;
}

static inline SrcModifier translate(GED_SRC_MOD mod) {
  SrcModifier srcMod;

  switch (mod) {
  case GED_SRC_MOD_Normal:
    srcMod = SrcModifier::NONE;
    break;
  case GED_SRC_MOD_Absolute:
    srcMod = SrcModifier::ABS;
    break;
  case GED_SRC_MOD_Negative:
    srcMod = SrcModifier::NEG;
    break;
  case GED_SRC_MOD_Negative_Absolute:
    srcMod = SrcModifier::NEG_ABS;
    break;
  case GED_SRC_MOD_INVALID:
  default:
    srcMod = SrcModifier::NONE;
    break;
  }
  return srcMod;
}

static inline DstModifier translate(GED_SATURATE mod) {
  DstModifier dstMod;

  switch (mod) {
  case GED_SATURATE_Normal:
    dstMod = DstModifier::NONE;
    break;
  case GED_SATURATE_sat:
    dstMod = DstModifier::SAT;
    break;
  case GED_SATURATE_INVALID:
  default:
    dstMod = DstModifier::NONE;
    break;
  }
  return dstMod;
}

static inline MathMacroExt translate(GED_MATH_MACRO_EXT mme) {
  switch (mme) {
  case GED_MATH_MACRO_EXT_mme0:
    return MathMacroExt::MME0;
  case GED_MATH_MACRO_EXT_mme1:
    return MathMacroExt::MME1;
  case GED_MATH_MACRO_EXT_mme2:
    return MathMacroExt::MME2;
  case GED_MATH_MACRO_EXT_mme3:
    return MathMacroExt::MME3;
  case GED_MATH_MACRO_EXT_mme4:
    return MathMacroExt::MME4;
  case GED_MATH_MACRO_EXT_mme5:
    return MathMacroExt::MME5;
  case GED_MATH_MACRO_EXT_mme6:
    return MathMacroExt::MME6;
  case GED_MATH_MACRO_EXT_mme7:
    return MathMacroExt::MME7;
  case GED_MATH_MACRO_EXT_nomme:
    return MathMacroExt::NOMME;
  default:
    return MathMacroExt::INVALID;
  }
}

static inline Type translate(GED_DATA_TYPE type) {
  Type opndType;

  switch (type) {
  case GED_DATA_TYPE_ud:
    opndType = Type::UD;
    break;
  case GED_DATA_TYPE_d:
    opndType = Type::D;
    break;
  case GED_DATA_TYPE_uw:
    opndType = Type::UW;
    break;
  case GED_DATA_TYPE_w:
    opndType = Type::W;
    break;
  case GED_DATA_TYPE_ub:
    opndType = Type::UB;
    break;
  case GED_DATA_TYPE_b:
    opndType = Type::B;
    break;
  case GED_DATA_TYPE_df:
    opndType = Type::DF;
    break;
  case GED_DATA_TYPE_f:
    opndType = Type::F;
    break;
  case GED_DATA_TYPE_uq:
    opndType = Type::UQ;
    break;
  case GED_DATA_TYPE_q:
    opndType = Type::Q;
    break;
  case GED_DATA_TYPE_hf:
    opndType = Type::HF;
    break;
  case GED_DATA_TYPE_uv:
    opndType = Type::UV;
    break;
  case GED_DATA_TYPE_vf:
    opndType = Type::VF;
    break;
  case GED_DATA_TYPE_v:
    opndType = Type::V;
    break;
  case GED_DATA_TYPE_nf:
    opndType = Type::NF;
    break;

  case GED_DATA_TYPE_bf:
    opndType = Type::BF;
    break;
  case GED_DATA_TYPE_qf:
    opndType = Type::QF;
    break;
  case GED_DATA_TYPE_bf8:
    opndType = Type::BF8;
    break;
  case GED_DATA_TYPE_hf8:
    opndType = Type::HF8;
    break;
  case GED_DATA_TYPE_tf32:
    opndType = Type::TF32;
    break;

  case GED_DATA_TYPE_INVALID:
  default:
    opndType = Type::INVALID;
    break;
  }
  return opndType;
}

static inline ChannelOffset translate(GED_CHANNEL_OFFSET ctrl) {
  ChannelOffset mOffset = ChannelOffset::M0;

  switch (ctrl) {
  case GED_CHANNEL_OFFSET_M0:
    mOffset = ChannelOffset::M0;
    break;
  case GED_CHANNEL_OFFSET_M4:
    mOffset = ChannelOffset::M4;
    break;
  case GED_CHANNEL_OFFSET_M8:
    mOffset = ChannelOffset::M8;
    break;
  case GED_CHANNEL_OFFSET_M12:
    mOffset = ChannelOffset::M12;
    break;
  case GED_CHANNEL_OFFSET_M16:
    mOffset = ChannelOffset::M16;
    break;
  case GED_CHANNEL_OFFSET_M20:
    mOffset = ChannelOffset::M20;
    break;
  case GED_CHANNEL_OFFSET_M24:
    mOffset = ChannelOffset::M24;
    break;
  case GED_CHANNEL_OFFSET_M28:
    mOffset = ChannelOffset::M28;
    break;
  default:
    break;
  }
  return mOffset;
}

// TODO: remove this an retain only translate<GED_CHANNEL_OFFSET>
static inline ChannelOffset translate(GED_EXEC_MASK_OFFSET_CTRL ctrl) {
  assert(ctrl != GED_EXEC_MASK_OFFSET_CTRL_INVALID);
  ChannelOffset mOffset = ChannelOffset::M0;

  switch (ctrl) {
  case GED_EXEC_MASK_OFFSET_CTRL_N1:
  case GED_EXEC_MASK_OFFSET_CTRL_Q1:
  case GED_EXEC_MASK_OFFSET_CTRL_H1:
    mOffset = ChannelOffset::M0;
    break;
  case GED_EXEC_MASK_OFFSET_CTRL_N3:
  case GED_EXEC_MASK_OFFSET_CTRL_Q2:
    mOffset = ChannelOffset::M8;
    break;
  case GED_EXEC_MASK_OFFSET_CTRL_N5:
  case GED_EXEC_MASK_OFFSET_CTRL_Q3:
  case GED_EXEC_MASK_OFFSET_CTRL_H2:
    mOffset = ChannelOffset::M16;
    break;
  case GED_EXEC_MASK_OFFSET_CTRL_N7:
  case GED_EXEC_MASK_OFFSET_CTRL_Q4:
    mOffset = ChannelOffset::M24;
    break;
  case GED_EXEC_MASK_OFFSET_CTRL_N2:
    mOffset = ChannelOffset::M4;
    break;
  case GED_EXEC_MASK_OFFSET_CTRL_N4:
    mOffset = ChannelOffset::M12;
    break;
  case GED_EXEC_MASK_OFFSET_CTRL_N6:
    mOffset = ChannelOffset::M20;
    break;
  case GED_EXEC_MASK_OFFSET_CTRL_N8:
    mOffset = ChannelOffset::M28;
    break;
  case GED_EXEC_MASK_OFFSET_CTRL_INVALID:
    mOffset = ChannelOffset::M0;
    break;
  }
  return mOffset;
}

static inline MaskCtrl translate(GED_MASK_CTRL cntrl) {
  MaskCtrl mCtrl;

  switch (cntrl) {
  case GED_MASK_CTRL_Normal:
    mCtrl = MaskCtrl::NORMAL;
    break;
  case GED_MASK_CTRL_NoMask:
    mCtrl = MaskCtrl::NOMASK;
    break;
  case GED_MASK_CTRL_INVALID:
  default:
    mCtrl = MaskCtrl::NORMAL;
    break;
  }

  return mCtrl;
}

static inline ExecSize translateExecSize(uint32_t size) {
  return ExecSizeFromInt((uint32_t)size);
}

static inline FlagModifier translate(GED_COND_MODIFIER mod) {
  switch (mod) {
  case GED_COND_MODIFIER_Normal:
    return FlagModifier::NONE;
  case GED_COND_MODIFIER_z:
    return FlagModifier::EQ;
  case GED_COND_MODIFIER_nz:
    return FlagModifier::NE;
  case GED_COND_MODIFIER_g:
    return FlagModifier::GT;
  case GED_COND_MODIFIER_ge:
    return FlagModifier::GE;
  case GED_COND_MODIFIER_l:
    return FlagModifier::LT;
  case GED_COND_MODIFIER_le:
    return FlagModifier::LE;
  case GED_COND_MODIFIER_o:
    return FlagModifier::OV;
  case GED_COND_MODIFIER_u:
    return FlagModifier::UN;
  default:
    return FlagModifier::NONE;
  }
}

static inline SFID translate(GED_SFID fc) {
  switch (fc) {
  case GED_SFID_NULL:
    return SFID::NULL_;
  case GED_SFID_SAMPLER:
    return SFID::SMPL;
  case GED_SFID_GATEWAY:
    return SFID::GTWY;
  case GED_SFID_DP_DC2:
    return SFID::DC2; // SKL+
  case GED_SFID_DP_RC:
    return SFID::RC;
  case GED_SFID_URB:
    return SFID::URB;
  case GED_SFID_SPAWNER:
    return SFID::TS;
  case GED_SFID_VME:
    return SFID::VME;
  case GED_SFID_DP_DCRO:
    return SFID::DCRO;
  case GED_SFID_DP_DC0:
    return SFID::DC0;
  case GED_SFID_PI:
    return SFID::PIXI;
  case GED_SFID_DP_DC1:
    return SFID::DC1;
  case GED_SFID_CRE:
    return SFID::CRE;
  case GED_SFID_DP_SAMPLER:
    return SFID::SMPL; // Pre-SKL
  case GED_SFID_DP_CC:
    return SFID::DCRO; // Pre-SKL
  case GED_SFID_BTD:
    return SFID::BTD;
  case GED_SFID_RTA:
    return SFID::RTA;
  case GED_SFID_TGM:
    return SFID::TGM;
  case GED_SFID_SLM:
    return SFID::SLM;
  case GED_SFID_UGM:
    return SFID::UGM;
  case GED_SFID_UGML:
    return SFID::UGML;
  default:
    return SFID::INVALID;
  }
}

static inline SFMessageType translate(GED_MESSAGE_TYPE fc) {
  switch (fc) {
  case GED_MESSAGE_TYPE_MSD0R_HWB:
    return SFMessageType::MSD0R_HWB;
  case GED_MESSAGE_TYPE_MSD0W_HWB:
    return SFMessageType::MSD0W_HWB;
  case GED_MESSAGE_TYPE_MT0R_OWB:
    return SFMessageType::MT0R_OWB;
  case GED_MESSAGE_TYPE_MT0R_OWAB:
    return SFMessageType::MT0R_OWUB;
  case GED_MESSAGE_TYPE_MT0R_OWDB:
    return SFMessageType::MT0R_OWDB;
  case GED_MESSAGE_TYPE_MT0R_DWS:
    return SFMessageType::MT0R_DWS;
  case GED_MESSAGE_TYPE_MT0R_BS:
    return SFMessageType::MT0R_BS;
  case GED_MESSAGE_TYPE_MT0_MEMFENCE:
    return SFMessageType::MT0_MEMFENCE;
  case GED_MESSAGE_TYPE_MT0W_OWB:
    return SFMessageType::MT0W_OWB;
  case GED_MESSAGE_TYPE_MT0W_OWDB:
    return SFMessageType::MT0W_OWDB;
  case GED_MESSAGE_TYPE_MT0W_DWS:
    return SFMessageType::MT0W_DWS;
  case GED_MESSAGE_TYPE_MT0W_BS:
    return SFMessageType::MT0W_BS;
  case GED_MESSAGE_TYPE_MT1R_T:
    return SFMessageType::MT1R_T;
  case GED_MESSAGE_TYPE_MT1R_US:
    return SFMessageType::MT1R_US;
  case GED_MESSAGE_TYPE_MT1A_UI:
    return SFMessageType::MT1A_UI;
  case GED_MESSAGE_TYPE_MT1A_UI4x2:
    return SFMessageType::MT1A_UI4x2;
  case GED_MESSAGE_TYPE_MT1R_MB:
    return SFMessageType::MT1R_MB;
  case GED_MESSAGE_TYPE_MT1R_TS:
    return SFMessageType::MT1R_TS;
  case GED_MESSAGE_TYPE_MT1A_TA:
    return SFMessageType::MT1A_TA;
  case GED_MESSAGE_TYPE_MT1A_TA4x2:
    return SFMessageType::MT1A_TA4x2;
  case GED_MESSAGE_TYPE_MT1W_US:
    return SFMessageType::MT1W_US;
  case GED_MESSAGE_TYPE_MT1W_MB:
    return SFMessageType::MT1W_MB;
  case GED_MESSAGE_TYPE_MT1A_TC:
    return SFMessageType::MT1A_TC;
  case GED_MESSAGE_TYPE_MT1A_TC4x2:
    return SFMessageType::MT1A_TC4x2;
  case GED_MESSAGE_TYPE_MT1W_TS:
    return SFMessageType::MT1W_TS;
  case GED_MESSAGE_TYPE_MT1R_A64_SB:
    return SFMessageType::MT1R_A64_SB;
  case GED_MESSAGE_TYPE_MT1R_A64_US:
    return SFMessageType::MT1R_A64_US;
  case GED_MESSAGE_TYPE_MT1A_A64_UI:
    return SFMessageType::MT1A_A64_UI;
  case GED_MESSAGE_TYPE_MT1A_A64_UI4x2:
    return SFMessageType::MT1A_A64_UI4x2;
  case GED_MESSAGE_TYPE_MT1R_A64_B:
    return SFMessageType::MT1R_A64_B;
  case GED_MESSAGE_TYPE_MT1W_A64_B:
    return SFMessageType::MT1W_A64_B;
  case GED_MESSAGE_TYPE_MT1W_A64_US:
    return SFMessageType::MT1W_A64_US;
  case GED_MESSAGE_TYPE_MT1W_A64_SB:
    return SFMessageType::MT1W_A64_SB;
  case GED_MESSAGE_TYPE_MT2R_US:
    return SFMessageType::MT2R_US;
  case GED_MESSAGE_TYPE_MT2R_A64_SB:
    return SFMessageType::MT2R_A64_SB;
  case GED_MESSAGE_TYPE_MT2R_A64_US:
    return SFMessageType::MT2R_A64_US;
  case GED_MESSAGE_TYPE_MT2R_BS:
    return SFMessageType::MT2R_BS;
  case GED_MESSAGE_TYPE_MT2W_US:
    return SFMessageType::MT2W_US;
  case GED_MESSAGE_TYPE_MT2W_A64_US:
    return SFMessageType::MT2W_A64_US;
  case GED_MESSAGE_TYPE_MT2W_A64_SB:
    return SFMessageType::MT2W_A64_SB;
  case GED_MESSAGE_TYPE_MT2W_BS:
    return SFMessageType::MT2W_BS;
  case GED_MESSAGE_TYPE_MT_CC_OWB:
    return SFMessageType::MT_CC_OWB;
  case GED_MESSAGE_TYPE_MT_CC_OWUB:
    return SFMessageType::MT_CC_OWUB;
  case GED_MESSAGE_TYPE_MT_CC_OWDB:
    return SFMessageType::MT_CC_OWDB;
  case GED_MESSAGE_TYPE_MT_CC_DWS:
    return SFMessageType::MT_CC_DWS;
  case GED_MESSAGE_TYPE_MT_SC_OWUB:
    return SFMessageType::MT_SC_OWUB;
  case GED_MESSAGE_TYPE_MT_SC_MB:
    return SFMessageType::MT_SC_MB;
  case GED_MESSAGE_TYPE_MT_RSI:
    return SFMessageType::MT_RSI;
  case GED_MESSAGE_TYPE_MT_RTW:
    return SFMessageType::MT_RTW;
  case GED_MESSAGE_TYPE_MT_RTR:
    return SFMessageType::MT_RTR;
  case GED_MESSAGE_TYPE_MTR_MB:
    return SFMessageType::MTR_MB;
  case GED_MESSAGE_TYPE_MTRR_TS:
    return SFMessageType::MTRR_TS;
  case GED_MESSAGE_TYPE_MTRA_TA:
    return SFMessageType::MTRA_TA;
  case GED_MESSAGE_TYPE_MT_MEMFENCE:
    return SFMessageType::MT_MEMFENCE;
  case GED_MESSAGE_TYPE_MTW_MB:
    return SFMessageType::MTW_MB;
  case GED_MESSAGE_TYPE_MTRW_TS:
    return SFMessageType::MTRW_TS;
  case GED_MESSAGE_TYPE_MT0R_US:
    return SFMessageType::MT0R_US;
  case GED_MESSAGE_TYPE_MT0A_UI:
    return SFMessageType::MT0A_UI;
  case GED_MESSAGE_TYPE_MT0W_US:
    return SFMessageType::MT0W_US;
  case GED_MESSAGE_TYPE_MT1A_UF4x2:
    return SFMessageType::MT1A_UF4x2;
  case GED_MESSAGE_TYPE_MT1A_UF:
    return SFMessageType::MT1A_UF;
  case GED_MESSAGE_TYPE_MT1A_A64_UF:
    return SFMessageType::MT1A_A64_UF;
  case GED_MESSAGE_TYPE_MT1A_A64_UF4x2:
    return SFMessageType::MT1A_A64_UF4x2;
  default:
    return SFMessageType::INVALID; // GED_MESSAGE_TYPE_INVALID
  }
}


static inline MathFC translate(GED_MATH_FC fc) {
  switch (fc) {
  case GED_MATH_FC_INV:
    return MathFC::INV;
  case GED_MATH_FC_LOG:
    return MathFC::LOG;
  case GED_MATH_FC_EXP:
    return MathFC::EXP;
  case GED_MATH_FC_SQRT:
    return MathFC::SQT;
  case GED_MATH_FC_RSQ:
    return MathFC::RSQT;
  case GED_MATH_FC_SIN:
    return MathFC::SIN;
  case GED_MATH_FC_COS:
    return MathFC::COS;
  case GED_MATH_FC_FDIV:
    return MathFC::FDIV;
  case GED_MATH_FC_POW:
    return MathFC::POW;
  case GED_MATH_FC_INT_DIV_BOTH:
    return MathFC::IDIV;
  case GED_MATH_FC_INT_DIV_QUOTIENT:
    return MathFC::IQOT;
  case GED_MATH_FC_INT_DIV_REMAINDER:
    return MathFC::IREM;
  case GED_MATH_FC_INVM:
    return MathFC::INVM;
  case GED_MATH_FC_RSQRTM:
    return MathFC::RSQTM;
  default:
    return MathFC::INVALID;
  }
}

static inline BranchCntrl translate(GED_BRANCH_CTRL c) {
  return c == GED_BRANCH_CTRL_Branch ? BranchCntrl::ON : BranchCntrl::OFF;
}

static inline bool translate(GED_PRED_INV inv) {
  return inv == GED_PRED_INV_Invert;
}

static inline Region::Vert translateRgnV(uint32_t stride) {
  Region::Vert vStride;

  switch (stride) {
  case 0:
    vStride = Region::Vert::VT_0;
    break;
  case 1:
    vStride = Region::Vert::VT_1;
    break;
  case 2:
    vStride = Region::Vert::VT_2;
    break;
    // GED uses 3 instead of 15 for VxH
  case 3:
    vStride = Region::Vert::VT_VxH;
    break;
  case 4:
    vStride = Region::Vert::VT_4;
    break;
  case 8:
    vStride = Region::Vert::VT_8;
    break;
  case 16:
    vStride = Region::Vert::VT_16;
    break;
  case 32:
    vStride = Region::Vert::VT_32;
    break;
  default:
    vStride = Region::Vert::VT_INVALID;
    break;
  }

  return vStride;
}

static inline Region::Width translateRgnW(uint32_t w) {
  Region::Width width;
  switch (w) {
  case 1:
    width = Region::Width::WI_1;
    break;
  case 2:
    width = Region::Width::WI_2;
    break;
  case 4:
    width = Region::Width::WI_4;
    break;
  case 8:
    width = Region::Width::WI_8;
    break;
  case 16:
    width = Region::Width::WI_16;
    break;
  default:
    width = Region::Width::WI_INVALID;
    break;
  }
  return width;
}

static inline Region::Horz translateRgnH(uint32_t stride) {
  Region::Horz hStride;
  switch (stride) {
  case 0:
    hStride = Region::Horz::HZ_0;
    break;
  case 1:
    hStride = Region::Horz::HZ_1;
    break;
  case 2:
    hStride = Region::Horz::HZ_2;
    break;
  case 4:
    hStride = Region::Horz::HZ_4;
    break;
  default:
    hStride = Region::Horz::HZ_INVALID;
    break;
  }
  return hStride;
}

static inline Region transateGEDtoIGARegion(uint32_t v, uint32_t w,
                                            uint32_t h) {
  Region val;
  val.set(translateRgnV(v), translateRgnW(w), translateRgnH(h));
  return val;
}

static inline SyncFC translate(GED_SYNC_FC fc) {
  switch (fc) {
  case GED_SYNC_FC_nop:
    return SyncFC::NOP;
  case GED_SYNC_FC_allrd:
    return SyncFC::ALLRD;
  case GED_SYNC_FC_allwr:
    return SyncFC::ALLWR;
  case GED_SYNC_FC_bar:
    return SyncFC::BAR;
  case GED_SYNC_FC_flush:
    return SyncFC::FLUSH;
  case GED_SYNC_FC_fence:
    return SyncFC::FENCE;
  case GED_SYNC_FC_host:
    return SyncFC::HOST;
  default:
    return SyncFC::INVALID;
  }
}
} // namespace iga

#endif
