/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_TO_GED_TRANSLATION_H_
#define _IGA_TO_GED_TRANSLATION_H_

#include "../../IR/Types.hpp"
#include "ged.h"

namespace iga {
static inline GED_OPCODE lowerOpcode(Op opcode) {
  GED_OPCODE op = GED_OPCODE_INVALID;
  switch (opcode) {
  case Op::ADD:
    op = GED_OPCODE_add;
    break;
  case Op::ADD3:
    op = GED_OPCODE_add3;
    break;
  case Op::ADDC:
    op = GED_OPCODE_addc;
    break;
  case Op::AND:
    op = GED_OPCODE_and;
    break;
  case Op::ASR:
    op = GED_OPCODE_asr;
    break;
  case Op::AVG:
    op = GED_OPCODE_avg;
    break;
  case Op::BFREV:
    op = GED_OPCODE_bfrev;
    break;
  case Op::BFE:
    op = GED_OPCODE_bfe;
    break;
  case Op::BFI1:
    op = GED_OPCODE_bfi1;
    break;
  case Op::BFI2:
    op = GED_OPCODE_bfi2;
    break;
  case Op::BFN:
    op = GED_OPCODE_bfn;
    break;
  case Op::BRC:
    op = GED_OPCODE_brc;
    break;
  case Op::BRD:
    op = GED_OPCODE_brd;
    break;
  case Op::BREAK:
    op = GED_OPCODE_break;
    break;
  case Op::CALL:
    op = GED_OPCODE_call;
    break;
  case Op::CALLA:
    op = GED_OPCODE_calla;
    break;
  case Op::CBIT:
    op = GED_OPCODE_cbit;
    break;
  case Op::CMP:
    op = GED_OPCODE_cmp;
    break;
  case Op::CMPN:
    op = GED_OPCODE_cmpn;
    break;
  case Op::CONT:
    op = GED_OPCODE_cont;
    break;
  case Op::CSEL:
    op = GED_OPCODE_csel;
    break;
  case Op::DPAS:
    op = GED_OPCODE_dpas;
    break;
  case Op::DPASW:
    op = GED_OPCODE_dpasw;
    break;
  case Op::DP2:
    op = GED_OPCODE_dp2;
    break;
  case Op::DP3:
    op = GED_OPCODE_dp3;
    break;
  case Op::DP4:
    op = GED_OPCODE_dp4;
    break;
  case Op::DP4A:
    op = GED_OPCODE_dp4a;
    break;
  case Op::DPH:
    op = GED_OPCODE_dph;
    break;
  case Op::ELSE:
    op = GED_OPCODE_else;
    break;
  case Op::ENDIF:
    op = GED_OPCODE_endif;
    break;
  case Op::F16TO32:
    op = GED_OPCODE_f16to32;
    break;
  case Op::F32TO16:
    op = GED_OPCODE_f32to16;
    break;
  case Op::FBH:
    op = GED_OPCODE_fbh;
    break;
  case Op::FBL:
    op = GED_OPCODE_fbl;
    break;
  case Op::FRC:
    op = GED_OPCODE_frc;
    break;
  case Op::GOTO:
    op = GED_OPCODE_goto;
    break;
  case Op::HALT:
    op = GED_OPCODE_halt;
    break;
  case Op::IF:
    op = GED_OPCODE_if;
    break;
  case Op::ILLEGAL:
    op = GED_OPCODE_illegal;
    break;
  case Op::JMPI:
    op = GED_OPCODE_jmpi;
    break;
  case Op::JOIN:
    op = GED_OPCODE_join;
    break;
  case Op::LINE:
    op = GED_OPCODE_line;
    break;
  case Op::LRP:
    op = GED_OPCODE_lrp;
    break;
  case Op::LZD:
    op = GED_OPCODE_lzd;
    break;
  case Op::MAC:
    op = GED_OPCODE_mac;
    break;
  case Op::MACH:
    op = GED_OPCODE_mach;
    break;
  case Op::MAD:
    op = GED_OPCODE_mad;
    break;
  case Op::MACL:
    op = GED_OPCODE_macl;
    break;
  case Op::SRND:
    op = GED_OPCODE_srnd;
    break;
  case Op::MADM:
    op = GED_OPCODE_madm;
    break;
  case Op::MATH:
    op = GED_OPCODE_math;
    break;
  case Op::MOV:
    op = GED_OPCODE_mov;
    break;
  case Op::MOVI:
    op = GED_OPCODE_movi;
    break;
  case Op::MUL:
    op = GED_OPCODE_mul;
    break;
  case Op::NOP:
    op = GED_OPCODE_nop;
    break;
  case Op::NOT:
    op = GED_OPCODE_not;
    break;
  case Op::OR:
    op = GED_OPCODE_or;
    break;
  case Op::PLN:
    op = GED_OPCODE_pln;
    break;
  case Op::RNDU:
    op = GED_OPCODE_rndu;
    break;
  case Op::RNDD:
    op = GED_OPCODE_rndd;
    break;
  case Op::RNDE:
    op = GED_OPCODE_rnde;
    break;
  case Op::RNDZ:
    op = GED_OPCODE_rndz;
    break;
  case Op::RET:
    op = GED_OPCODE_ret;
    break;
  case Op::ROR:
    op = GED_OPCODE_ror;
    break;
  case Op::ROL:
    op = GED_OPCODE_rol;
    break;
  case Op::SAD2:
    op = GED_OPCODE_sad2;
    break;
  case Op::SADA2:
    op = GED_OPCODE_sada2;
    break;
  case Op::SEL:
    op = GED_OPCODE_sel;
    break;
  //
  case Op::SEND:
    op = GED_OPCODE_send;
    break;
  case Op::SENDC:
    op = GED_OPCODE_sendc;
    break;
  case Op::SENDS:
    op = GED_OPCODE_sends;
    break;
  case Op::SENDSC:
    op = GED_OPCODE_sendsc;
    break;
  case Op::SHL:
    op = GED_OPCODE_shl;
    break;
  case Op::SHR:
    op = GED_OPCODE_shr;
    break;
  case Op::SMOV:
    op = GED_OPCODE_smov;
    break;
  case Op::SUBB:
    op = GED_OPCODE_subb;
    break;
  //
  case Op::SYNC:
    op = GED_OPCODE_sync;
    break;
  case Op::WAIT:
    op = GED_OPCODE_wait;
    break;
  case Op::WHILE:
    op = GED_OPCODE_while;
    break;
  case Op::XOR:
    op = GED_OPCODE_xor;
    break;
  //
  default:
    break;
  }

  return op;
}

static inline GED_MODEL lowerPlatform(Platform platform) {
  GED_MODEL pltf = GED_MODEL_INVALID;
  switch (platform) {
  case Platform::GEN7:
    pltf = GED_MODEL_7;
    break;
  case Platform::GEN7P5:
    pltf = GED_MODEL_7_5;
    break;
  case Platform::GEN8:
    pltf = GED_MODEL_8;
    break;
  case Platform::GEN8LP:
    pltf = GED_MODEL_8_1;
    break;
  case Platform::GEN9:
    pltf = GED_MODEL_9;
    break;
  case Platform::GEN9LP:
    pltf = GED_MODEL_9;
    break;
  case Platform::GEN9P5:
    pltf = GED_MODEL_9;
    break;
  case Platform::GEN10:
    pltf = GED_MODEL_10;
    break;
  case Platform::GEN11:
    pltf = GED_MODEL_11;
    break;
  case Platform::XE:
    pltf = GED_MODEL_TGL;
    break;
  case Platform::XE_HP:
    pltf = GED_MODEL_XE_HP;
    break;
  case Platform::XE_HPG:
    pltf = GED_MODEL_XE_HPG;
    break;
  case Platform::XE_HPC:
    pltf = GED_MODEL_XE_HPC;
    break;
  case Platform::XE2:
    pltf = GED_MODEL_XE2;
    break;
  case Platform::XE3:
    pltf = GED_MODEL_XE3;
    break;
  default:
    break;
  }

  return pltf;
}

static inline GED_SYNC_FC lowerSyncFC(SyncFC fc) {
  switch (fc) {
  case SyncFC::ALLRD:
    return GED_SYNC_FC_allrd;
  case SyncFC::ALLWR:
    return GED_SYNC_FC_allwr;
  case SyncFC::BAR:
    return GED_SYNC_FC_bar;
  case SyncFC::FENCE:
    return GED_SYNC_FC_fence;
  case SyncFC::FLUSH:
    return GED_SYNC_FC_flush;
  case SyncFC::HOST:
    return GED_SYNC_FC_host;
  case SyncFC::NOP:
    return GED_SYNC_FC_nop;
  default:
    return GED_SYNC_FC_INVALID;
  }
}


static inline GED_PRED_CTRL lowerPredCtrl(PredCtrl predMod) {
  GED_PRED_CTRL predCtrl = GED_PRED_CTRL_INVALID;

  switch (predMod) {
  case PredCtrl::NONE:
    predCtrl = GED_PRED_CTRL_Normal;
    break;
  case PredCtrl::SEQ:
    predCtrl = GED_PRED_CTRL_Sequential;
    break;
  case PredCtrl::ANY2H:
    predCtrl = GED_PRED_CTRL_any2h;
    break;
  case PredCtrl::ANY4H:
    predCtrl = GED_PRED_CTRL_any4h;
    break;
  case PredCtrl::ANY8H:
    predCtrl = GED_PRED_CTRL_any8h;
    break;
  case PredCtrl::ANY16H:
    predCtrl = GED_PRED_CTRL_any16h;
    break;
  case PredCtrl::ANY32H:
    predCtrl = GED_PRED_CTRL_any32h;
    break;
  case PredCtrl::ALL2H:
    predCtrl = GED_PRED_CTRL_all2h;
    break;
  case PredCtrl::ALL4H:
    predCtrl = GED_PRED_CTRL_all4h;
    break;
  case PredCtrl::ALL8H:
    predCtrl = GED_PRED_CTRL_all8h;
    break;
  case PredCtrl::ALL16H:
    predCtrl = GED_PRED_CTRL_all16h;
    break;
  case PredCtrl::ALL32H:
    predCtrl = GED_PRED_CTRL_all32h;
    break;
  case PredCtrl::ANYV:
    predCtrl = GED_PRED_CTRL_anyv;
    break;
  case PredCtrl::ALLV:
    predCtrl = GED_PRED_CTRL_allv;
    break;
  case PredCtrl::ANY:
    predCtrl = GED_PRED_CTRL_any;
    break;
  case PredCtrl::ALL:
    predCtrl = GED_PRED_CTRL_all;
    break;
  default:
    break;
  }

  return predCtrl;
}

static inline GED_SRC_MOD lowerSrcMod(SrcModifier modSource) {
  GED_SRC_MOD srcMod = GED_SRC_MOD_INVALID;

  switch (modSource) {
  case SrcModifier::NONE:
    srcMod = GED_SRC_MOD_Normal;
    break;
  case SrcModifier::ABS:
    srcMod = GED_SRC_MOD_Absolute;
    break;
  case SrcModifier::NEG:
    srcMod = GED_SRC_MOD_Negative;
    break;
  case SrcModifier::NEG_ABS:
    srcMod = GED_SRC_MOD_Negative_Absolute;
    break;
  default:
    break;
  }

  return srcMod;
}

static inline GED_SFID lowerSFID(SFID igaSfid) {
  GED_SFID sfid = GED_SFID_INVALID;
  switch (igaSfid) {
  case SFID::CRE:
    sfid = GED_SFID_CRE;
    break;
  case SFID::DC0:
    sfid = GED_SFID_DP_DC0;
    break;
  case SFID::DC1:
    sfid = GED_SFID_DP_DC1;
    break;
  case SFID::DC2:
    sfid = GED_SFID_DP_DC2;
    break;
  case SFID::DCRO:
    sfid = GED_SFID_DP_DCRO;
    break;
  case SFID::GTWY:
    sfid = GED_SFID_GATEWAY;
    break;
  case SFID::NULL_:
    sfid = GED_SFID_NULL;
    break;
  case SFID::PIXI:
    sfid = GED_SFID_PI;
    break;
  case SFID::SMPL:
    sfid = GED_SFID_SAMPLER;
    break;
  case SFID::RC:
    sfid = GED_SFID_DP_RC;
    break;
  case SFID::TS:
    sfid = GED_SFID_SPAWNER;
    break;
  case SFID::URB:
    sfid = GED_SFID_URB;
    break;
  case SFID::VME:
    sfid = GED_SFID_VME;
    break;
  case SFID::BTD:
    sfid = GED_SFID_BTD;
    break;
  case SFID::RTA:
    sfid = GED_SFID_RTA;
    break;
  case SFID::TGM:
    sfid = GED_SFID_TGM;
    break;
  case SFID::SLM:
    sfid = GED_SFID_SLM;
    break;
  case SFID::UGM:
    sfid = GED_SFID_UGM;
    break;
  case SFID::UGML:
    sfid = GED_SFID_UGML;
    break;
  default:
    break;
  }
  return sfid;
}

static inline GED_SFID lowerSendTFID(uint32_t id, Platform platform) {
  GED_SFID tfid = GED_SFID_INVALID;
  switch (id) {
  case 0:
    tfid = GED_SFID_NULL;
    break;
  case 2:
    tfid = GED_SFID_SAMPLER;
    break;
  case 3:
    tfid = GED_SFID_GATEWAY;
    break;
  case 4:
    if (platform < Platform::GEN9) {
      tfid = GED_SFID_DP_SAMPLER;
    } else {
      tfid = GED_SFID_DP_DC2;
    }
    break;
  case 5:
    tfid = GED_SFID_DP_RC;
    break;
  case 6:
    tfid = GED_SFID_URB;
    break;
  case 7:
    tfid = GED_SFID_SPAWNER;
    if (platform >= Platform::XE_HPG) {
      tfid = GED_SFID_BTD;
    }
    break;
  case 8:
    tfid = GED_SFID_VME;
    if (platform >= Platform::XE_HPG) {
      tfid = GED_SFID_RTA;
    }
    break;
  case 9:
    if (platform < Platform::GEN9) {
      tfid = GED_SFID_DP_CC;
    } else {
      tfid = GED_SFID_DP_DCRO;
    }
    break;
  case 10:
    tfid = GED_SFID_DP_DC0;
    break;
  case 11:
    tfid = GED_SFID_PI;
    break;
  case 12:
    tfid = GED_SFID_DP_DC1;
    break;
  case 13:
    tfid = GED_SFID_CRE;
    if (platform >= Platform::XE_HPG) {
      tfid = GED_SFID_TGM;
    }
    break;
  case 14:
    tfid = GED_SFID_SLM;
    break;
  case 15:
    tfid = GED_SFID_INVALID;
    if (platform >= Platform::XE_HPG) {
      tfid = GED_SFID_UGM;
    }
    break;
  default:
    break;
  }
  return tfid;
}

static inline GED_SATURATE lowerSaturate(DstModifier dstSat) {
  GED_SATURATE gedSat = GED_SATURATE_INVALID;

  switch (dstSat) {
  case DstModifier::NONE:
    gedSat = GED_SATURATE_Normal;
    break;
  case DstModifier::SAT:
    gedSat = GED_SATURATE_sat;
    break;
  default:
    break;
  }

  return gedSat;
}

static inline GED_EXECUTION_DATA_TYPE lowerExecDataType(Type opndType) {
  GED_EXECUTION_DATA_TYPE type = GED_EXECUTION_DATA_TYPE_INVALID;

  switch (opndType) {
  case Type::HF:
  case Type::BF:
  case Type::F:
  case Type::DF:
  case Type::NF:
  case Type::BF8:
  case Type::TF32:
    type = GED_EXECUTION_DATA_TYPE::GED_EXECUTION_DATA_TYPE_Float;
    break;
  default:
    type = GED_EXECUTION_DATA_TYPE::GED_EXECUTION_DATA_TYPE_Integer;
    break;
  }
  return type;
}

static inline GED_DATA_TYPE lowerDataType(Type opndType) {
  GED_DATA_TYPE dataType = GED_DATA_TYPE_INVALID;

  switch (opndType) {
  case Type::B:
    dataType = GED_DATA_TYPE_b;
    break;
  case Type::D:
    dataType = GED_DATA_TYPE_d;
    break;
  case Type::DF:
    dataType = GED_DATA_TYPE_df;
    break;
  case Type::F:
    dataType = GED_DATA_TYPE_f;
    break;
  case Type::HF:
    dataType = GED_DATA_TYPE_hf;
    break;
  case Type::NF:
    dataType = GED_DATA_TYPE_nf;
    break;
  case Type::Q:
    dataType = GED_DATA_TYPE_q;
    break;
  case Type::UB:
    dataType = GED_DATA_TYPE_ub;
    break;
  case Type::UD:
    dataType = GED_DATA_TYPE_ud;
    break;
  case Type::UQ:
    dataType = GED_DATA_TYPE_uq;
    break;
  case Type::UV:
    dataType = GED_DATA_TYPE_uv;
    break;
  case Type::UW:
    dataType = GED_DATA_TYPE_uw;
    break;
  case Type::V:
    dataType = GED_DATA_TYPE_v;
    break;
  case Type::VF:
    dataType = GED_DATA_TYPE_vf;
    break;
  case Type::W:
    dataType = GED_DATA_TYPE_w;
    break;
  case Type::BF:
    dataType = GED_DATA_TYPE_bf;
    break;
  case Type::S2:
  case Type::S4:
    dataType = GED_DATA_TYPE_b;
    break;
  // case Type::U1: no mapping as of today
  case Type::U2:
  case Type::U4:
    dataType = GED_DATA_TYPE_ub;
    break;
  case Type::QF:
    dataType = GED_DATA_TYPE_qf;
    break;
  case Type::BF8:
    dataType = GED_DATA_TYPE_bf8;
    break;
  case Type::HF8:
    dataType = GED_DATA_TYPE_hf8;
    break;
  case Type::TF32:
    dataType = GED_DATA_TYPE_tf32;
    break;
  default:
    break;
  }
  return dataType;
}

static inline GED_PRECISION lowerSubBytePrecision(Type t) {
  GED_PRECISION precision = GED_PRECISION_INVALID;
  switch (t) {
  case Type::U1:
    precision = GED_PRECISION_u1;
    break;
  case Type::U2:
    precision = GED_PRECISION_u2;
    break;
  case Type::U4:
    precision = GED_PRECISION_u4;
    break;
  case Type::UB:
    precision = GED_PRECISION_u8;
    break;
  case Type::S2:
    precision = GED_PRECISION_s2;
    break;
  case Type::S4:
    precision = GED_PRECISION_s4;
    break;
  case Type::B:
    precision = GED_PRECISION_s8;
    break;
  case Type::BF:
    precision = GED_PRECISION_bf16;
    break;
  case Type::HF:
    precision = GED_PRECISION_f16;
    break;
  case Type::BF8:
    precision = GED_PRECISION_bf8;
    break;
  case Type::TF32:
    precision = GED_PRECISION_tf32;
    break;
  case Type::HF8:
    precision = GED_PRECISION_hf8;
    break;
  default:
    break;
  }
  return precision;
}

static inline GED_MATH_MACRO_EXT lowerSpecialAcc(MathMacroExt mme) {
  switch (mme) {
  case MathMacroExt::MME0:
    return GED_MATH_MACRO_EXT_mme0;
  case MathMacroExt::MME1:
    return GED_MATH_MACRO_EXT_mme1;
  case MathMacroExt::MME2:
    return GED_MATH_MACRO_EXT_mme2;
  case MathMacroExt::MME3:
    return GED_MATH_MACRO_EXT_mme3;
  case MathMacroExt::MME4:
    return GED_MATH_MACRO_EXT_mme4;
  case MathMacroExt::MME5:
    return GED_MATH_MACRO_EXT_mme5;
  case MathMacroExt::MME6:
    return GED_MATH_MACRO_EXT_mme6;
  case MathMacroExt::MME7:
    return GED_MATH_MACRO_EXT_mme7;
  case MathMacroExt::NOMME:
    return GED_MATH_MACRO_EXT_nomme;
  default:
    return GED_MATH_MACRO_EXT_INVALID;
  }
}

static inline GED_CHANNEL_OFFSET lowerQtrCtrl(ChannelOffset ectrl) {
  GED_CHANNEL_OFFSET gedCtrl = GED_CHANNEL_OFFSET_INVALID;
  switch (ectrl) {
  case ChannelOffset::M0:
    gedCtrl = GED_CHANNEL_OFFSET_M0;
    break;
  case ChannelOffset::M4:
    gedCtrl = GED_CHANNEL_OFFSET_M4;
    break;
  case ChannelOffset::M8:
    gedCtrl = GED_CHANNEL_OFFSET_M8;
    break;
  case ChannelOffset::M12:
    gedCtrl = GED_CHANNEL_OFFSET_M12;
    break;
  case ChannelOffset::M16:
    gedCtrl = GED_CHANNEL_OFFSET_M16;
    break;
  case ChannelOffset::M20:
    gedCtrl = GED_CHANNEL_OFFSET_M20;
    break;
  case ChannelOffset::M24:
    gedCtrl = GED_CHANNEL_OFFSET_M24;
    break;
  case ChannelOffset::M28:
    gedCtrl = GED_CHANNEL_OFFSET_M28;
    break;
  default:
    break;
  }
  return gedCtrl;
}

static inline GED_EXEC_MASK_OFFSET_CTRL lowerQtrCtrl(ExecSize eSize,
                                                     ChannelOffset ectrl) {
  GED_EXEC_MASK_OFFSET_CTRL gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_INVALID;
  switch (eSize) {
  case ExecSize::SIMD1:
  case ExecSize::SIMD2:
    if (ectrl == ChannelOffset::M0)
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_Q1;
    switch (ectrl) {
    case ChannelOffset::M0:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_Q1;
      break;
    case ChannelOffset::M8:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_Q2;
      break;
    case ChannelOffset::M16:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_Q3;
      break;
    case ChannelOffset::M24:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_Q4;
      break;
    default:
      break;
    }
    break;
  case ExecSize::SIMD4:
    switch (ectrl) {
    case ChannelOffset::M0:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_N1;
      break;
    case ChannelOffset::M4:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_N2;
      break;
    case ChannelOffset::M8:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_N3;
      break;
    case ChannelOffset::M12:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_N4;
      break;
    case ChannelOffset::M16:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_N5;
      break;
    case ChannelOffset::M20:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_N6;
      break;
    case ChannelOffset::M24:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_N7;
      break;
    case ChannelOffset::M28:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_N8;
      break;
    default:
      break;
    }
    break;
  case ExecSize::SIMD8:
    switch (ectrl) {
    case ChannelOffset::M0:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_Q1;
      break;
    case ChannelOffset::M8:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_Q2;
      break;
    case ChannelOffset::M16:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_Q3;
      break;
    case ChannelOffset::M24:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_Q4;
      break;
    default:
      break;
    }
    break;
  case ExecSize::SIMD16:
    switch (ectrl) {
    case ChannelOffset::M0:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_H1;
      break;
    case ChannelOffset::M16:
      gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_H2;
      break;
    default:
      break;
    }
    break;
  case ExecSize::SIMD32:
    gedCtrl = GED_EXEC_MASK_OFFSET_CTRL_Q1;
    break;
  default:
    break;
  }
  return gedCtrl;
}

static inline GED_MASK_CTRL lowerEmask(MaskCtrl ectrl) {
  GED_MASK_CTRL eMask = GED_MASK_CTRL_INVALID;

  switch (ectrl) {
  case MaskCtrl::NORMAL:
    eMask = GED_MASK_CTRL_Normal;
    break;
  case MaskCtrl::NOMASK:
    eMask = GED_MASK_CTRL_NoMask;
    break;
  default:
    break;
  }

  return eMask;
}

static inline uint32_t lowerExecSize(ExecSize eSize) {
  return (uint32_t)ExecSizeToInt(eSize);
}

static inline GED_COND_MODIFIER lowerCondModifier(FlagModifier condMod) {
  GED_COND_MODIFIER mod = GED_COND_MODIFIER_INVALID;

  switch (condMod) {
  case FlagModifier::NONE:
    mod = GED_COND_MODIFIER_Normal;
    break;
  case FlagModifier::EQ:
    mod = GED_COND_MODIFIER_z;
    break;
  case FlagModifier::NE:
    mod = GED_COND_MODIFIER_nz;
    break;
  case FlagModifier::GT:
    mod = GED_COND_MODIFIER_g;
    break;
  case FlagModifier::GE:
    mod = GED_COND_MODIFIER_ge;
    break;
  case FlagModifier::LT:
    mod = GED_COND_MODIFIER_l;
    break;
  case FlagModifier::LE:
    mod = GED_COND_MODIFIER_le;
    break;
  case FlagModifier::OV:
    mod = GED_COND_MODIFIER_o;
    break;
  case FlagModifier::UN:
    mod = GED_COND_MODIFIER_u;
    break;
  default:
    break;
  }

  return mod;
}


static inline GED_MATH_FC lowerMathFC(MathFC fc) {
  switch (fc) {
  case MathFC::INV:
    return GED_MATH_FC_INV;
    break;
  case MathFC::LOG:
    return GED_MATH_FC_LOG;
    break;
  case MathFC::EXP:
    return GED_MATH_FC_EXP;
    break;
  case MathFC::SQT:
    return GED_MATH_FC_SQRT;
    break;
  case MathFC::RSQT:
    return GED_MATH_FC_RSQ;
    break;
  case MathFC::POW:
    return GED_MATH_FC_POW;
    break;
  case MathFC::SIN:
    return GED_MATH_FC_SIN;
    break;
  case MathFC::COS:
    return GED_MATH_FC_COS;
    break;
  case MathFC::IDIV:
    return GED_MATH_FC_INT_DIV_BOTH;
    break;
  case MathFC::IQOT:
    return GED_MATH_FC_INT_DIV_QUOTIENT;
    break;
  case MathFC::IREM:
    return GED_MATH_FC_INT_DIV_REMAINDER;
    break;
  case MathFC::FDIV:
    return GED_MATH_FC_FDIV;
    break;
  case MathFC::INVM:
    return GED_MATH_FC_INVM;
    break;
  case MathFC::RSQTM:
    return GED_MATH_FC_RSQRTM;
    break;
  default:
    return GED_MATH_FC_INV;
  }
}

static inline GED_BRANCH_CTRL lowerBranchCntrl(BranchCntrl brnch) {
  GED_BRANCH_CTRL gedBrnch = GED_BRANCH_CTRL_INVALID;

  switch (brnch) {
  case BranchCntrl::OFF:
    gedBrnch = GED_BRANCH_CTRL_Normal;
    break;
  case BranchCntrl::ON:
    gedBrnch = GED_BRANCH_CTRL_Branch;
    break;
  default:
    break;
  }

  return gedBrnch;
}

static inline GED_REG_FILE lowerRegFile(RegName type) {
  return (type == RegName::GRF_R) ? GED_REG_FILE_GRF : GED_REG_FILE_ARF;
}

static inline GED_MATH_MACRO_EXT lowerMathMacroReg(MathMacroExt MathMacroReg) {
  // NOTE: GED puts special accumulators as acc2 to acc9
  switch (MathMacroReg) {
  case MathMacroExt::MME0:
    return GED_MATH_MACRO_EXT_mme0;
  case MathMacroExt::MME1:
    return GED_MATH_MACRO_EXT_mme1;
  case MathMacroExt::MME2:
    return GED_MATH_MACRO_EXT_mme2;
  case MathMacroExt::MME3:
    return GED_MATH_MACRO_EXT_mme3;
  case MathMacroExt::MME4:
    return GED_MATH_MACRO_EXT_mme4;
  case MathMacroExt::MME5:
    return GED_MATH_MACRO_EXT_mme5;
  case MathMacroExt::MME6:
    return GED_MATH_MACRO_EXT_mme6;
  case MathMacroExt::MME7:
    return GED_MATH_MACRO_EXT_mme7;
  case MathMacroExt::NOMME:
    return GED_MATH_MACRO_EXT_nomme;
  default:
    return GED_MATH_MACRO_EXT_INVALID;
  }
  return GED_MATH_MACRO_EXT_INVALID;
}

static inline GED_ARCH_REG lowerArchReg(RegName type) {
  GED_ARCH_REG archReg;

  switch (type) {
  case RegName::ARF_NULL:
    archReg = GED_ARCH_REG_null;
    break;
  case RegName::ARF_A:
    archReg = GED_ARCH_REG_a0;
    break;
  case RegName::ARF_ACC:
    archReg = GED_ARCH_REG_acc;
    break;
  case RegName::ARF_F:
    archReg = GED_ARCH_REG_f;
    break;
  case RegName::ARF_CE:
    archReg = GED_ARCH_REG_ce;
    break;
  case RegName::ARF_MSG:
    archReg = GED_ARCH_REG_msg;
    break;
  case RegName::ARF_SP:
    archReg = GED_ARCH_REG_sp;
    break;
  case RegName::ARF_S:
    archReg = GED_ARCH_REG_s;
    break;
  case RegName::ARF_SR:
    archReg = GED_ARCH_REG_sr0;
    break;
  case RegName::ARF_CR:
    archReg = GED_ARCH_REG_cr0;
    break;
  case RegName::ARF_N:
    archReg = GED_ARCH_REG_n0;
    break;
  case RegName::ARF_IP:
    archReg = GED_ARCH_REG_ip;
    break;
  case RegName::ARF_TDR:
    archReg = GED_ARCH_REG_tdr;
    break;
  case RegName::ARF_TM:
    archReg = GED_ARCH_REG_tm0;
    break;
  case RegName::ARF_FC:
    archReg = GED_ARCH_REG_fc;
    break;
  case RegName::ARF_DBG:
    archReg = GED_ARCH_REG_dbg0;
    break;
  case RegName::GRF_R:
    archReg = GED_ARCH_REG_INVALID;
    break;
  default:
    archReg = GED_ARCH_REG_INVALID;
    break;
  }
  return archReg;
}

static inline uint32_t lowerRegionVert(const Region::Vert vt) {
  return vt == Region::Vert::VT_VxH ? 3 : static_cast<uint32_t>(vt);
}

static inline uint32_t lowerRegionWidth(const Region::Width wi) {
  return static_cast<uint32_t>(wi);
}

static inline uint32_t lowerRegionHorz(const Region::Horz hz) {
  return static_cast<uint32_t>(hz);
}

static inline uint32_t createChanSel(GED_SWIZZLE swizzleX, GED_SWIZZLE swizzleY,
                                     GED_SWIZZLE swizzleZ,
                                     GED_SWIZZLE swizzleW) {
  uint32_t chanSel = swizzleX;
  chanSel |= swizzleY << 2;
  chanSel |= swizzleZ << 4;
  chanSel |= swizzleW << 6;
  return chanSel;
}
} // namespace iga

#endif
