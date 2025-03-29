/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Common_ISA_util.h"
#include "Common_ISA_framework.h"
#include "G4_Opcode.h"
#include "PreDefinedVars.h"
#include <sstream>
#include <string.h>
#include "visa_igc_common_header.h"
#include "Assertions.h"

using namespace vISA;

vISAPreDefinedSurface vISAPreDefSurf[COMMON_ISA_NUM_PREDEFINED_SURF_VER_3_1] = {
    {0, PREDEF_SURF_0, "%slm"},   {1, PREDEF_SURF_1, "T1"},
    {2, PREDEF_SURF_2, "T2"},     {3, PREDEF_SURF_3, "TSS"},
    {4, PREDEF_SURF_252, "%bss"}, {5, PREDEF_SURF_255, "%scratch"},
};

typedef struct {
  VISA_Align CISAAlign;
  const char *AlignName;
  uint16_t AlignBytes;
} CISAAlignInfo;

static CISAAlignInfo CISAAlignTable[ALIGN_TOTAL_NUM] = {
    {ALIGN_BYTE, "byte", 1},       {ALIGN_WORD, "word", 2},
    {ALIGN_DWORD, "dword", 4},     {ALIGN_QWORD, "qword", 8},
    {ALIGN_OWORD, "oword", 16},    {ALIGN_GRF, "GRF", 0}, // dynamic
    {ALIGN_2_GRF, "GRFx2", 0},                            // dynamic
    {ALIGN_HWORD, "hword", 32},    {ALIGN_32WORD, "wordx32", 64},
    {ALIGN_64WORD, "wordx64", 128}};

const char *Common_ISA_Get_Align_Name(VISA_Align align) {
  vASSERT(align < ARRAY_COUNT(CISAAlignTable));
  return CISAAlignTable[align].AlignName;
}

uint32_t getAlignInBytes(VISA_Align A, unsigned grfSize) {
  switch (A) {
  case ALIGN_GRF:
    return grfSize;
  case ALIGN_2_GRF:
    return 2 * grfSize;
  default:
    break;
  }
  vASSERT(A < ARRAY_COUNT(CISAAlignTable));
  return CISAAlignTable[A].AlignBytes;
}

VISA_Align getCISAAlign(uint32_t AlignInBytes) {
  if (AlignInBytes >= 128)
    return ALIGN_64WORD;
  if (AlignInBytes >= 64)
    return ALIGN_32WORD;
  if (AlignInBytes >= 32)
    return ALIGN_HWORD;
  if (AlignInBytes >= 16)
    return ALIGN_OWORD;
  if (AlignInBytes >= 8)
    return ALIGN_QWORD;
  if (AlignInBytes >= 4)
    return ALIGN_DWORD;
  if (AlignInBytes >= 2)
    return ALIGN_WORD;
  return ALIGN_BYTE;
}

GenPrecision_Info_t GenPrecisionTable[] = {
    /*  0 */ {GenPrecision::INVALID, 0, nullptr},
    /*  1 */ {GenPrecision::U1, 1, "u1"},
    /*  2 */ {GenPrecision::S1, 1, "s1"},
    /*  3 */ {GenPrecision::U2, 2, "u2"},
    /*  4 */ {GenPrecision::S2, 2, "s2"},
    /*  5 */ {GenPrecision::U4, 4, "u4"},
    /*  6 */ {GenPrecision::S4, 4, "s4"},
    /*  7 */ {GenPrecision::U8, 8, "u8"},
    /*  8 */ {GenPrecision::S8, 8, "s8"},
    /*  9 */ {GenPrecision::BF16, 16, "bf"},
    /* 10 */ {GenPrecision::FP16, 16, "hf"},
    /* 11 */ {GenPrecision::BF8, 8, "bf8"},
    /* 12 */ {GenPrecision::TF32, 32, "tf32"},
    /* 13 */ {GenPrecision::INVALID, 0, nullptr}, // unused
    /* 14 */ {GenPrecision::HF8, 8, "hf8"},
};
static_assert((int)GenPrecision::INVALID == 0);
static_assert((int)GenPrecision::U1 == 1);
static_assert((int)GenPrecision::S1 == 2);
static_assert((int)GenPrecision::U2 == 3);
static_assert((int)GenPrecision::S2 == 4);
static_assert((int)GenPrecision::U4 == 5);
static_assert((int)GenPrecision::S4 == 6);
static_assert((int)GenPrecision::U8 == 7);
static_assert((int)GenPrecision::S8 == 8);
static_assert((int)GenPrecision::BF16 == 9);
static_assert((int)GenPrecision::FP16 == 10);
static_assert((int)GenPrecision::BF8 == 11);
static_assert((int)GenPrecision::TF32 == 12);
static_assert((int)GenPrecision::HF8 == 14);
static_assert((int)GenPrecision::TOTAL_NUM == 15);

const char *Common_ISA_Get_Modifier_Name(VISA_Modifier modifier) {
  switch (modifier) {
  case MODIFIER_NONE:
    return "";
  case MODIFIER_ABS:
    return "(abs)";
  case MODIFIER_NEG:
    return "(-)";
  case MODIFIER_NEG_ABS:
    return "(-abs)";
  case MODIFIER_SAT:
    return "sat";
  case MODIFIER_NOT:
    return "(~)";
  default:
    vISA_ASSERT_UNREACHABLE("Invalid modifier.");
    return "invalid_modifier";
  }
}

short Common_ISA_Get_Region_Value(Common_ISA_Region_Val val) {
  switch (val) {
  case REGION_NULL:
    return -1;
  case REGION_0:
    return 0;
  case REGION_1:
    return 1;
  case REGION_2:
    return 2;
  case REGION_4:
    return 4;
  case REGION_8:
    return 8;
  case REGION_16:
    return 16;
  case REGION_32:
    return 32;
  default:
    std::stringstream ss;
    ss << " illegal region value: " << (int)val;
    vISA_ASSERT_UNREACHABLE(ss.str());
    return -1;
  }
}

G4_opcode GetGenOpcodeFromVISAOpcode(ISA_Opcode opcode) {
  switch (opcode) {
  case ISA_RESERVED_0:
    return G4_illegal;
  case ISA_ADD:
    return G4_add;
  case ISA_AVG:
    return G4_avg;
  case ISA_DIV:
    return G4_math;
  case ISA_DP2:
    return G4_dp2;
  case ISA_DP3:
    return G4_dp3;
  case ISA_DP4:
    return G4_dp4;
  case ISA_DPH:
    return G4_dph;
  case ISA_DP4A:
    return G4_dp4a;
  case ISA_DPAS:
    return G4_dpas;
  case ISA_DPASW:
    return G4_dpasw;
  case ISA_ADD3:
  case ISA_ADD3O:
    return G4_add3;
  case ISA_BFN:
    return G4_bfn;
  case ISA_FCVT:
    return G4_fcvt;
  case ISA_SRND:
    return G4_srnd;
  case ISA_EXP:
    return G4_math;
  case ISA_FRC:
    return G4_frc;
  case ISA_LINE:
    return G4_line;
  case ISA_LOG:
    return G4_math;
  case ISA_LRP:
    return G4_lrp;
  case ISA_MAD:
    return G4_pseudo_mad;
  case ISA_MOD:
    return G4_math;
  case ISA_MUL:
    return G4_mul;
  case ISA_PLANE:
    return G4_pln;
  case ISA_POW:
    return G4_math;
  case ISA_RNDD:
    return G4_rndd;
  case ISA_RNDE:
    return G4_rnde;
  case ISA_RNDU:
    return G4_rndu;
  case ISA_RNDZ:
    return G4_rndz;
  case ISA_SAD2:
    return G4_sad2;
  case ISA_SAD2ADD:
    return G4_pseudo_sada2;
  case ISA_SIN:
    return G4_math;
  case ISA_COS:
    return G4_math;
  case ISA_SQRT:
    return G4_math;
  case ISA_RSQRT:
    return G4_math;
  case ISA_INV:
    return G4_math;
  case ISA_LZD:
    return G4_lzd;
  case ISA_AND:
    return G4_and;
  case ISA_OR:
    return G4_or;
  case ISA_XOR:
    return G4_xor;
  case ISA_NOT:
    return G4_not;
  case ISA_SHL:
    return G4_shl;
  case ISA_SHR:
    return G4_shr;
  case ISA_ASR:
    return G4_asr;
  case ISA_ROL:
    return G4_rol;
  case ISA_ROR:
    return G4_ror;
  case ISA_BFE:
    return G4_bfe;
  case ISA_BFI:
    return G4_bfi1;
  case ISA_BFREV:
    return G4_bfrev;
  case ISA_CBIT:
    return G4_cbit;
  case ISA_FBL:
    return G4_fbl;
  case ISA_FBH:
    return G4_fbh;
  case ISA_ADDR_ADD:
    return G4_add;
  case ISA_MOV:
    return G4_mov;
  case ISA_SEL:
  case ISA_FMINMAX:
    return G4_sel;
  case ISA_SETP:
    break;
  case ISA_CMP:
    return G4_cmp;
  case ISA_SUBROUTINE:
    break;
  case ISA_LABEL:
    return G4_label;
  case ISA_JMP:
    return G4_jmpi;
  case ISA_CALL:
    return G4_call;
  case ISA_RET:
    return G4_return;
  case ISA_MULH:
    return G4_mulh;
  case ISA_ADDC:
    return G4_addc;
  case ISA_SUBB:
    return G4_subb;
  case ISA_OWORD_LD:
  case ISA_OWORD_ST:
  case ISA_MEDIA_LD:
  case ISA_MEDIA_ST:
  case ISA_GATHER:
  case ISA_SCATTER:
  case ISA_OWORD_LD_UNALIGNED:
  case ISA_SAMPLE:
  case ISA_SAMPLE_UNORM:
  case ISA_FILE:
  case ISA_LOC:
  case ISA_DWORD_ATOMIC:
    break;
  case ISA_GOTO:
    return G4_goto;
  case ISA_MADW:
    return G4_madw;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("Invalid opcode in common ISA.");
    break;
  }
  return G4_illegal;
}

G4_Type GetGenTypeFromVISAType(VISA_Type type) {
  switch (type) {
  case ISA_TYPE_UD:
    return Type_UD;
  case ISA_TYPE_D:
    return Type_D;
  case ISA_TYPE_UW:
    return Type_UW;
  case ISA_TYPE_W:
    return Type_W;
  case ISA_TYPE_UB:
    return Type_UB;
  case ISA_TYPE_B:
    return Type_B;
  case ISA_TYPE_DF:
    return Type_DF;
  case ISA_TYPE_F:
    return Type_F;
  case ISA_TYPE_VF:
    return Type_VF;
  case ISA_TYPE_V:
    return Type_V;
  case ISA_TYPE_BOOL:
    return Type_BOOL;
  case ISA_TYPE_UV:
    return Type_UV;
  case ISA_TYPE_Q:
    return Type_Q;
  case ISA_TYPE_UQ:
    return Type_UQ;
  case ISA_TYPE_HF:
    return Type_HF;
  case ISA_TYPE_BF:
    return Type_BF;
  default:
    return Type_UNDEF;
  }
}

VISA_Type Get_Common_ISA_Type_From_G4_Type(G4_Type type) {
  switch (type) {
  case Type_UD:
    return ISA_TYPE_UD;
  case Type_D:
    return ISA_TYPE_D;
  case Type_UW:
    return ISA_TYPE_UW;
  case Type_W:
    return ISA_TYPE_W;
  case Type_UB:
    return ISA_TYPE_UB;
  case Type_B:
    return ISA_TYPE_B;
  case Type_DF:
    return ISA_TYPE_DF;
  case Type_F:
    return ISA_TYPE_F;
  case Type_VF:
    return ISA_TYPE_VF;
  case Type_V:
    return ISA_TYPE_V;
  case Type_BOOL:
    return ISA_TYPE_BOOL;
  case Type_UV:
    return ISA_TYPE_UV;
  case Type_Q:
    return ISA_TYPE_Q;
  case Type_UQ:
    return ISA_TYPE_UQ;
  case Type_HF:
    return ISA_TYPE_HF;
  case Type_BF:
    return ISA_TYPE_BF;
  default:
    return ISA_TYPE_NUM;
  }
}

G4_SrcModifier GetGenSrcModFromVISAMod(VISA_Modifier mod) {
  switch (mod) {
  case MODIFIER_NONE:
    return Mod_src_undef;
  case MODIFIER_ABS:
    return Mod_Abs;
  case MODIFIER_NEG:
    return Mod_Minus;
  case MODIFIER_NEG_ABS:
    return Mod_Minus_Abs;
  case MODIFIER_NOT:
    return Mod_Not;
  default:
    vISA_ASSERT_UNREACHABLE("Wrong src modifier");
    return Mod_src_undef;
  }
}

G4_CondModifier
Get_G4_CondModifier_From_Common_ISA_CondModifier(VISA_Cond_Mod cmod) {
  switch (cmod) {
  case ISA_CMP_E:
    return Mod_e;
  case ISA_CMP_NE:
    return Mod_ne;
  case ISA_CMP_G:
    return Mod_g;
  case ISA_CMP_GE:
    return Mod_ge;
  case ISA_CMP_L:
    return Mod_l;
  case ISA_CMP_LE:
    return Mod_le;
  case ISA_CMP_UNDEF:
    return Mod_cond_undef;
  default:
    vISA_ASSERT_UNREACHABLE("Invalid CISA Conditional Modifier.");
    return Mod_cond_undef;
  }
}

bool hasPredicate(ISA_Opcode op) {
  switch (ISA_Inst_Table[op].type) {
  case ISA_Inst_Mov:
    return !(op == ISA_SETP || op == ISA_MOVS || op == ISA_FMINMAX ||
             op == ISA_FCVT);
  case ISA_Inst_Arith:
  case ISA_Inst_Logic: {
    if (op == ISA_SRND) {
      return false;
    }
    return true;
  }
  case ISA_Inst_Compare:
  case ISA_Inst_Address:
  case ISA_Inst_Data_Port:
  case ISA_Inst_Sampler:
  case ISA_Inst_Misc:
    return (op == ISA_DWORD_ATOMIC || op == ISA_GATHER_SCALED ||
            op == ISA_GATHER4_SCALED || op == ISA_GATHER4_TYPED ||
            op == ISA_SCATTER_SCALED || op == ISA_SCATTER4_SCALED ||
            op == ISA_SCATTER4_TYPED || op == ISA_RAW_SEND ||
            op == ISA_RAW_SENDS || op == ISA_3D_SAMPLE || op == ISA_3D_LOAD ||
            op == ISA_3D_GATHER4 || op == ISA_3D_RT_WRITE ||
            op == ISA_3D_URB_WRITE || op == ISA_3D_TYPED_ATOMIC ||
            op == ISA_QW_GATHER || op == ISA_QW_SCATTER
    );
  case ISA_Inst_Flow:
    return !(op == ISA_SUBROUTINE || op == ISA_LABEL || op == ISA_SWITCHJMP);
  case ISA_Inst_LSC:
    return true;
  case ISA_Inst_SIMD_Flow:
    return op == ISA_GOTO;
  case ISA_Inst_SVM:
    return true;
  default:
    return false;
  }
}

bool hasExecSize(ISA_Opcode op, uint8_t subOp) {
  switch (ISA_Inst_Table[op].type) {
  case ISA_Inst_Mov:
  case ISA_Inst_Arith:
  case ISA_Inst_Logic:
  case ISA_Inst_Compare:
  case ISA_Inst_Address:
    return true;
  case ISA_Inst_Data_Port:
    if (op == ISA_MEDIA_LD || op == ISA_MEDIA_ST) {
      return false;
    } else
      return true;
  case ISA_Inst_SVM:
    if (subOp == SVM_BLOCK_LD || subOp == SVM_BLOCK_ST || subOp == 0) {
      return false;
    } else
      return true;
  case ISA_Inst_LSC:
    return true;
  case ISA_Inst_Sampler:
  case ISA_Inst_Misc:
    if (op == ISA_RAW_SEND || op == ISA_RAW_SENDS || op == ISA_3D_SAMPLE ||
        op == ISA_3D_LOAD || op == ISA_3D_GATHER4 || op == ISA_3D_URB_WRITE ||
        op == ISA_3D_INFO) {
      return true;
    } else if (op == ISA_DPAS || op == ISA_DPASW) {
      return true;
    } else {
      return false;
    }
  case ISA_Inst_Flow:
    if (op == ISA_SUBROUTINE || op == ISA_LABEL || op == ISA_FADDR) {
      return false;
    } else
      return true;
  case ISA_Inst_SIMD_Flow:
    return true;
  default:
    return false;
  }
}

bool hasLabelSrc(ISA_Opcode op) {
  if (ISA_Inst_Table[op].type == ISA_Inst_Flow) {
    if (op == ISA_RET || op == ISA_FRET || op == ISA_IFCALL || op == ISA_FADDR)
      return false;
    else //(op == ISA_SUBROUTINE || op == ISA_LABEL || op == ISA_JMP || op ==
         //ISA_CALL || op == ISA_FCALL)
      return true;
  } else if (op == ISA_GOTO)
    return true;
  else
    return false;
}

unsigned Get_Common_ISA_SVM_Block_Num(VISA_SVM_Block_Num num) {
  switch (num) {
  case SVM_BLOCK_NUM_1:
    return 1;
  case SVM_BLOCK_NUM_2:
    return 2;
  case SVM_BLOCK_NUM_4:
    return 4;
  case SVM_BLOCK_NUM_8:
    return 8;
  default:
    vISA_ASSERT_UNREACHABLE("Illegal SVM block number (should be 1, 2, 4, or 8).");
  }
  return 0;
}

VISA_SVM_Block_Num valueToVISASVMBlockNum(unsigned int value) {
  switch (value) {
  case 1:
    return SVM_BLOCK_NUM_1;
  case 2:
    return SVM_BLOCK_NUM_2;
  case 4:
    return SVM_BLOCK_NUM_4;
  case 8:
    return SVM_BLOCK_NUM_8;
  default:
    vISA_ASSERT_UNREACHABLE("invalid SVM block number");
    return SVM_BLOCK_NUM_1;
  }
}

VISA_SVM_Block_Type valueToVISASVMBlockType(unsigned int value) {
  switch (value) {
  case 1:
    return SVM_BLOCK_TYPE_BYTE;
  case 4:
    return SVM_BLOCK_TYPE_DWORD;
  case 8:
    return SVM_BLOCK_TYPE_QWORD;

  default:
    vISA_ASSERT_UNREACHABLE("invalid SVM block number");
    return SVM_BLOCK_TYPE_BYTE;
  }
}

unsigned Get_Common_ISA_SVM_Block_Size(VISA_SVM_Block_Type size) {
  switch (size) {
  case SVM_BLOCK_TYPE_BYTE:
    return 1;
  case SVM_BLOCK_TYPE_DWORD:
    return 4;
  case SVM_BLOCK_TYPE_QWORD:
    return 8;
  default:
    vISA_ASSERT_UNREACHABLE("Illegal SVM block size (should be 1, 4, or 8).");
  }
  return 0;
}

unsigned Get_VISA_Oword_Num(VISA_Oword_Num num) {
  switch (num) {
  case OWORD_NUM_1:
    return 1;
  case OWORD_NUM_2:
    return 2;
  case OWORD_NUM_4:
    return 4;
  case OWORD_NUM_8:
    return 8;
  case OWORD_NUM_16:
    return 16;
  default:
    vISA_ASSERT_UNREACHABLE("illegal Oword number (should be 0..3).");
    return 0;
  }
}

unsigned Get_VISA_Exec_Size(VISA_Exec_Size size) {
  switch (size) {
  case EXEC_SIZE_1:
    return 1;
  case EXEC_SIZE_2:
    return 2;
  case EXEC_SIZE_4:
    return 4;
  case EXEC_SIZE_8:
    return 8;
  case EXEC_SIZE_16:
    return 16;
  case EXEC_SIZE_32:
    return 32;
  default:
    vISA_ASSERT_UNREACHABLE("illegal common ISA execsize (should be 0..5).");
    return 0;
  }
}

bool IsMathInst(ISA_Opcode op) {
  switch (op) {
  case ISA_INV:
  case ISA_DIV:
  case ISA_MOD:
  case ISA_LOG:
  case ISA_EXP:
  case ISA_SQRT:
  case ISA_RSQRT:
  case ISA_SIN:
  case ISA_COS:
  case ISA_POW:
    return true;
  default:
    return false;
  }
}

bool IsIntType(VISA_Type type) {
  switch (type) {
  case ISA_TYPE_UD:
  case ISA_TYPE_D:
  case ISA_TYPE_UW:
  case ISA_TYPE_W:
  case ISA_TYPE_UB:
  case ISA_TYPE_B:
  case ISA_TYPE_Q:
  case ISA_TYPE_UQ:
    return true;
  default:
    return false;
  }
}

bool IsIntOrIntVecType(VISA_Type type) {
  return type == ISA_TYPE_V || type == ISA_TYPE_UV || IsIntType(type);
}

bool IsSingedIntType(VISA_Type type) {
  switch (type) {
  case ISA_TYPE_D:
  case ISA_TYPE_W:
  case ISA_TYPE_B:
  case ISA_TYPE_Q:
    return true;
  default:
    return false;
  }
}

bool IsUnsignedIntType(VISA_Type type) {
  switch (type) {
  case ISA_TYPE_UD:
  case ISA_TYPE_UW:
  case ISA_TYPE_UB:
  case ISA_TYPE_UQ:
    return true;
  default:
    return false;
  }
}

unsigned short Get_Common_ISA_Region_Value(Common_ISA_Region_Val val) {
  switch (val) {
  case REGION_0:
    return 0;
  case REGION_1:
    return 1;
  case REGION_2:
    return 2;
  case REGION_4:
    return 4;
  case REGION_8:
    return 8;
  case REGION_16:
    return 16;
  case REGION_32:
    return 32;
  default:
    return UNDEFINED_SHORT; //???
  }
}

Common_ISA_Region_Val Get_CISA_Region_Val(short val) {
  if (val == (short)0x8000) {
    return REGION_NULL;
  } else {
    switch (val) {
    case 0:
      return REGION_0;
    case 1:
      return REGION_1;
    case 2:
      return REGION_2;
    case 4:
      return REGION_4;
    case 8:
      return REGION_8;
    case 16:
      return REGION_16;
    case 32:
      return REGION_32;
    case -1:
      return REGION_NULL;
    default:
      vISA_ASSERT_UNREACHABLE("Invalid Region value.");
      return REGION_NULL;
    }
  }
}

unsigned short Create_CISA_Region(unsigned short vstride, unsigned short width,
                                  unsigned short hstride) {
  unsigned short region = 0;
  region |= (unsigned short)Get_CISA_Region_Val(vstride) & 0xF;
  region |= ((unsigned short)Get_CISA_Region_Val(width) & 0xF) << 4;
  region |= ((unsigned short)Get_CISA_Region_Val(hstride) & 0xF) << 8;
  return region;
}

unsigned Round_Down_Pow2(unsigned n) {
  unsigned int i = 1;
  while (n >= i)
    i <<= 1;
  return (i >> 1);
}
unsigned Round_Up_Pow2(unsigned n) {
  unsigned int i = 1;
  if (n == 0)
    return 0;
  while (n > i)
    i <<= 1;
  return i;
}

G4_opcode Get_Pseudo_Opcode(ISA_Opcode op) {
  switch (op) {
  case ISA_AND:
    return G4_pseudo_and;
  case ISA_OR:
    return G4_pseudo_or;
  case ISA_XOR:
    return G4_pseudo_xor;
  case ISA_NOT:
    return G4_pseudo_not;
  default:
    return G4_illegal;
  }
  return G4_illegal;
}

VISA_EMask_Ctrl Get_Next_EMask(VISA_EMask_Ctrl currEMask,
                               G4_ExecSize execSize) {
  switch (execSize) {
  default: // Next eMask is only valid for SIMD4, SIMD8, and SIMD16.
    break;
  case 16:
    switch (currEMask) {
    case vISA_EMASK_M1:
      return vISA_EMASK_M5;
    case vISA_EMASK_M1_NM:
      return vISA_EMASK_M5_NM;
    default:
      break;
    }
    break;
  case 8:
    switch (currEMask) {
    case vISA_EMASK_M1:
      return vISA_EMASK_M3;
    case vISA_EMASK_M1_NM:
      return vISA_EMASK_M3_NM;
    case vISA_EMASK_M3:
      return vISA_EMASK_M5;
    case vISA_EMASK_M3_NM:
      return vISA_EMASK_M5_NM;
    case vISA_EMASK_M5:
      return vISA_EMASK_M7;
    case vISA_EMASK_M5_NM:
      return vISA_EMASK_M7_NM;
    default:
      break;
    }
    break;
  case 4:
    switch (currEMask) {
    case vISA_EMASK_M1:
      return vISA_EMASK_M2;
    case vISA_EMASK_M1_NM:
      return vISA_EMASK_M2_NM;
    case vISA_EMASK_M2:
      return vISA_EMASK_M3;
    case vISA_EMASK_M2_NM:
      return vISA_EMASK_M3_NM;
    case vISA_EMASK_M3:
      return vISA_EMASK_M4;
    case vISA_EMASK_M3_NM:
      return vISA_EMASK_M4_NM;
    case vISA_EMASK_M4:
      return vISA_EMASK_M5;
    case vISA_EMASK_M4_NM:
      return vISA_EMASK_M5_NM;
    case vISA_EMASK_M5:
      return vISA_EMASK_M6;
    case vISA_EMASK_M5_NM:
      return vISA_EMASK_M6_NM;
    case vISA_EMASK_M6:
      return vISA_EMASK_M7;
    case vISA_EMASK_M6_NM:
      return vISA_EMASK_M7_NM;
    case vISA_EMASK_M7:
      return vISA_EMASK_M8;
    case vISA_EMASK_M7_NM:
      return vISA_EMASK_M8_NM;
    default:
      break;
    }
    break;
  }

  return vISA_NUM_EMASK;
}

G4_InstOpts Get_Gen4_Emask(VISA_EMask_Ctrl cisa_emask, G4_ExecSize exec_size) {

  switch (exec_size.value) {
  case 32:
    switch (cisa_emask) {
    case vISA_EMASK_M1:
      return InstOpt_NoOpt;
    case vISA_EMASK_M5:
      return InstOpt_M16;
    case vISA_EMASK_M1_NM:
      return InstOpt_WriteEnable;
    case vISA_EMASK_M5_NM:
      return InstOpt_M16 | InstOpt_WriteEnable;
    default:
      vISA_ASSERT_UNREACHABLE("Invalid emask for SIMD32 inst");
      return InstOpt_NoOpt;
    }
    break;
  case 16: {
    switch (cisa_emask) {
    case vISA_EMASK_M1:
      return InstOpt_M0;
    case vISA_EMASK_M5:
      return InstOpt_M16;
    case vISA_EMASK_M1_NM:
      return InstOpt_M0 | InstOpt_WriteEnable;
    case vISA_EMASK_M5_NM:
      return InstOpt_M16 | InstOpt_WriteEnable;
    default:
      vISA_ASSERT_UNREACHABLE("Invalid emask for SIMD16 inst");
      return InstOpt_NoOpt;
    }
  } break;
  case 8: {
    switch (cisa_emask) {
    case vISA_EMASK_M1:
      return InstOpt_M0;
    case vISA_EMASK_M3:
      return InstOpt_M8;
    case vISA_EMASK_M5:
      return InstOpt_M16;
    case vISA_EMASK_M7:
      return InstOpt_M24;
    case vISA_EMASK_M1_NM:
      return InstOpt_M0 | InstOpt_WriteEnable;
    case vISA_EMASK_M3_NM:
      return InstOpt_M8 | InstOpt_WriteEnable;
    case vISA_EMASK_M5_NM:
      return InstOpt_M16 | InstOpt_WriteEnable;
    case vISA_EMASK_M7_NM:
      return InstOpt_M24 | InstOpt_WriteEnable;
    default:
      vISA_ASSERT_UNREACHABLE("Invalid emask for SIMD8 inst");
      return InstOpt_NoOpt;
    }
  }
  default:
    // size 4, 2, 1
    {
      switch (cisa_emask) {
      case vISA_EMASK_M1:
        return InstOpt_M0;
      case vISA_EMASK_M2:
        return InstOpt_M4;
      case vISA_EMASK_M3:
        return InstOpt_M8;
      case vISA_EMASK_M4:
        return InstOpt_M12;
      case vISA_EMASK_M5:
        return InstOpt_M16;
      case vISA_EMASK_M6:
        return InstOpt_M20;
      case vISA_EMASK_M7:
        return InstOpt_M24;
      case vISA_EMASK_M8:
        return InstOpt_M28;
      case vISA_EMASK_M1_NM:
        return InstOpt_M0 | InstOpt_WriteEnable;
      case vISA_EMASK_M2_NM:
        return InstOpt_M4 | InstOpt_WriteEnable;
      case vISA_EMASK_M3_NM:
        return InstOpt_M8 | InstOpt_WriteEnable;
      case vISA_EMASK_M4_NM:
        return InstOpt_M12 | InstOpt_WriteEnable;
      case vISA_EMASK_M5_NM:
        return InstOpt_M16 | InstOpt_WriteEnable;
      case vISA_EMASK_M6_NM:
        return InstOpt_M20 | InstOpt_WriteEnable;
      case vISA_EMASK_M7_NM:
        return InstOpt_M24 | InstOpt_WriteEnable;
      case vISA_EMASK_M8_NM:
        return InstOpt_M28 | InstOpt_WriteEnable;
      default:
        vISA_ASSERT_UNREACHABLE("Invalid emask for SIMD4 inst.");
        return InstOpt_NoOpt;
      }
    }
  }
}

unsigned Get_Atomic_Op(VISAAtomicOps op) {

  switch (op) {
  default:
    vISA_ASSERT_UNREACHABLE("CISA error: Invalid vISA atomic op for DWord atomic write.");
    break;
  case ATOMIC_ADD:
    return GEN_ATOMIC_ADD;
  case ATOMIC_SUB:
    return GEN_ATOMIC_SUB;
  case ATOMIC_INC:
    return GEN_ATOMIC_INC;
  case ATOMIC_DEC:
    return GEN_ATOMIC_DEC;
  case ATOMIC_MIN:
    return GEN_ATOMIC_UMIN;
  case ATOMIC_MAX:
    return GEN_ATOMIC_UMAX;
  case ATOMIC_XCHG:
    return GEN_ATOMIC_MOV;
  case ATOMIC_CMPXCHG:
    return GEN_ATOMIC_CMPWR;
  case ATOMIC_AND:
    return GEN_ATOMIC_AND;
  case ATOMIC_OR:
    return GEN_ATOMIC_OR;
  case ATOMIC_XOR:
    return GEN_ATOMIC_XOR;
  case ATOMIC_IMIN:
    return GEN_ATOMIC_IMIN;
  case ATOMIC_IMAX:
    return GEN_ATOMIC_IMAX;
  case ATOMIC_PREDEC:
    return GEN_ATOMIC_PREDEC;
  case ATOMIC_FMIN:
    return GEN_ATOMIC_FMIN;
  case ATOMIC_FMAX:
    return GEN_ATOMIC_FMAX;
  case ATOMIC_FCMPWR:
    return GEN_ATOMIC_FCMPWR;
  case ATOMIC_FADD:
    return GEN_ATOMIC_FADD;
  case ATOMIC_FSUB:
    return GEN_ATOMIC_FSUB;
  }
  return ~0U;
}

uint16_t Get_VISA_Type_Size(VISA_Type type) {
  switch (type) {
  case ISA_TYPE_UD:
  case ISA_TYPE_D:
  case ISA_TYPE_F:
  case ISA_TYPE_V:
  case ISA_TYPE_VF:
  case ISA_TYPE_UV:
    return 4;
  case ISA_TYPE_UW:
  case ISA_TYPE_W:
  case ISA_TYPE_HF:
    return 2;
  case ISA_TYPE_UB:
  case ISA_TYPE_B:
  case ISA_TYPE_BOOL:
    return 1;
  case ISA_TYPE_DF:
  case ISA_TYPE_Q:
  case ISA_TYPE_UQ:
    return 8;
  default:
    vISA_ASSERT_UNREACHABLE("Invalid data type: size unknown.");
    return 0;
  }
}

int attribute_info_t::getSizeInBinary() const {
  return sizeof(nameIndex) + sizeof(size) + size;
}

int label_info_t::getSizeInBinary() const {
  int size = sizeof(name_index) + sizeof(kind) + sizeof(attribute_count);

  for (int i = 0; i < attribute_count; i++) {
    size += attributes[i].getSizeInBinary();
  }
  return size;
}

int var_info_t::getSizeInBinary() const {
  /*
      var_info {
  ud name_index;
  ub bit_properties;
  uw num_elements;
  ud alias_index;
  uw alias_offset;
  ub attribute_count;
  attribute_info[attribute_count];
  }
  */
  int size = sizeof(name_index) + sizeof(bit_properties) +
             sizeof(num_elements) + sizeof(alias_index) + sizeof(alias_offset) +
             sizeof(alias_scope_specifier) + sizeof(attribute_count);

  for (int i = 0; i < attribute_count; i++) {
    size += attributes[i].getSizeInBinary();
  }
  return size;
}

int addr_info_t::getSizeInBinary() const {
  /*
  address_info {
  ud name_index;
  uw num_elements;
  ub attribute_count;
  attribute_info[attribute_count];
  }
  */
  int size =
      sizeof(name_index) + sizeof(num_elements) + sizeof(attribute_count);
  for (int i = 0; i < attribute_count; i++) {
    size += attributes[i].getSizeInBinary();
  }
  return size;
}

int pred_info_t::getSizeInBinary() const {
  /*
  predicate_info {
  ud name_index;
  uw num_elements;
  ub attribute_count;
  attribute_info[attribute_count];
  }
  */
  int size =
      sizeof(name_index) + sizeof(num_elements) + sizeof(attribute_count);
  for (int i = 0; i < attribute_count; i++) {
    size += attributes[i].getSizeInBinary();
  }
  return size;
}

int input_info_t::getSizeInBinary() const {
  /*
  input_info {
  b kind;
  ud id;
  w offset;
  uw size;
  }
  */
  return sizeof(kind) + sizeof(index) + sizeof(offset) + sizeof(size);
}

int vector_opnd::getSizeInBinary() const {
  int size = 0;

  switch (tag & 0x7) {
  case OPERAND_GENERAL: {
    size =
        sizeof(opnd_val.gen_opnd.index) + sizeof(opnd_val.gen_opnd.col_offset) +
        sizeof(opnd_val.gen_opnd.row_offset) + sizeof(opnd_val.gen_opnd.region);
    break;
  }
  case OPERAND_ADDRESS: {
    size = sizeof(opnd_val.addr_opnd.index) +
           sizeof(opnd_val.addr_opnd.offset) + sizeof(opnd_val.addr_opnd.width);
    break;
  }
  case OPERAND_INDIRECT: {
    size = sizeof(opnd_val.indirect_opnd.index) +
           sizeof(opnd_val.indirect_opnd.addr_offset) +
           sizeof(opnd_val.indirect_opnd.indirect_offset) +
           sizeof(opnd_val.indirect_opnd.bit_property) +
           sizeof(opnd_val.indirect_opnd.region);
    break;
  }
  case OPERAND_PREDICATE: {
    size = sizeof(opnd_val.pred_opnd.index);
    break;
  }
  case OPERAND_IMMEDIATE: {
    switch (opnd_val.const_opnd.type) {
    default:
      size = sizeof(unsigned int);
      break;
    case ISA_TYPE_Q:
    case ISA_TYPE_UQ:
    case ISA_TYPE_DF:
      size = sizeof(unsigned long long);
      break;
    }

    size += sizeof(opnd_val.const_opnd.type);
    break;
  }
  case OPERAND_STATE: {
    size = sizeof(opnd_val.state_opnd.index) +
           sizeof(opnd_val.state_opnd.offset) +
           sizeof(opnd_val.state_opnd.opnd_class);
    break;
  }
  default: {
    vISA_ASSERT_UNREACHABLE("Invalid Vector Operand Class. Size cannot be determined.");
    break;
  }
  }

  size += sizeof(tag);

  return size;
}

/*
function_info {
    ub linkage; // MBZ
    uw name_len;
    ub name[name_len];
    ud offset;
    ud size;
    uw num_syms_variable; // MBZ
    uw num_syms_function; // MBZ
}
*/
uint32_t function_info_t::getSizeInBinary() const {
  uint32_t size = sizeof(linkage) + sizeof(name_len) + name_len +
                  sizeof(offset) + sizeof(this->size);

  size += sizeof(variable_reloc_symtab.num_syms);
  size += sizeof(function_reloc_symtab.num_syms);

  return size;
}

/*
    kernel_info {
    uw name_len;
    ub name[name_len];
    ud offset;
    ud size;
    ud input_offset;
    uw num_syms_variable; // MBZ
    uw num_syms_function; // MBZ
    ub num_gen_binaries;
    gen_binary_info gen_binaries[num_gen_binaries];
}
*/
uint32_t kernel_info_t::getSizeInBinary() const {
  uint32_t size = sizeof(name_len) + name_len + sizeof(offset) +
                  sizeof(this->size) + sizeof(input_offset);

  size += sizeof(variable_reloc_symtab.num_syms);
  size += sizeof(function_reloc_symtab.num_syms);

  size += sizeof(num_gen_binaries);

  for (int i = 0; i < num_gen_binaries; i++) {
    size += sizeof(gen_binaries->platform);
    size += sizeof(gen_binaries->binary_offset);
    size += sizeof(gen_binaries->binary_size);
  }

  return size;
}

uint32_t common_isa_header::getSizeInBinary() const {
  uint32_t size = sizeof(magic_number) + sizeof(major_version) +
                  sizeof(minor_version) + sizeof(num_kernels);

  for (int i = 0; i < num_kernels; i++) {
    size += kernels[i].getSizeInBinary();
  }
  /*
  common_isa_header {
  ud magic_number;
  ub major_version;
  ub minor_version;
  uw num_kernels;
  kernel_info kernels[num_kernels];
  uw num_variables;
  file_scope_var_info variables[num_variables];
  uw num_functions;
  function_info functions[num_functions];
  }

  */

  // file-scope variables are no longer supported
  size += sizeof(uint16_t);

  size += sizeof(num_functions);

  for (int i = 0; i < num_functions; i++) {
    size += functions[i].getSizeInBinary();
  }

  return size;
}

VISA_Cond_Mod
Get_Common_ISA_CondModifier_From_G4_CondModifier(G4_CondModifier cmod) {
  switch (cmod) {
  // case ISA_CMP_NONE:
  //     return Mod_z;
  case Mod_e:
    return ISA_CMP_E;
  case Mod_ne:
    return ISA_CMP_NE;
  case Mod_g:
    return ISA_CMP_G;
  case Mod_ge:
    return ISA_CMP_GE;
  case Mod_l:
    return ISA_CMP_L;
  case Mod_le:
    return ISA_CMP_LE;
  // case ISA_CMP_R:
  //     return Mod_r;
  // case ISA_CMP_O:
  //     return Mod_o;
  // case ISA_CMP_U:
  //     return Mod_u;
  case Mod_cond_undef:
    return ISA_CMP_UNDEF;
  default:
    vISA_ASSERT_UNREACHABLE("Invalid G4 Conditional Modifier.");
    return ISA_CMP_UNDEF;
  }
}

VISA_Exec_Size Get_VISA_Exec_Size_From_Raw_Size(unsigned int size) {
  switch (size) {
  case 1:
    return EXEC_SIZE_1;
  case 2:
    return EXEC_SIZE_2;
  case 4:
    return EXEC_SIZE_4;
  case 8:
    return EXEC_SIZE_8;
  case 16:
    return EXEC_SIZE_16;
  case 32:
    return EXEC_SIZE_32;
  default:
    vISA_ASSERT(false,
                 "illegal common ISA execsize (should be 1, 2, 4, 8, 16, 32).");
    return EXEC_SIZE_ILLEGAL;
  }
}

int state_info_t::getSizeInBinary() const {
  int size =
      sizeof(name_index) + sizeof(num_elements) + sizeof(attribute_count);

  for (int i = 0; i < attribute_count; i++) {
    size += attributes[i].getSizeInBinary();
  }
  return size;
}

VISA_Oword_Num Get_VISA_Oword_Num_From_Number(unsigned num) {
  switch (num) {
  case 1:
    return OWORD_NUM_1;
  case 2:
    return OWORD_NUM_2;
  case 4:
    return OWORD_NUM_4;
  case 8:
    return OWORD_NUM_8;
  case 16:
    return OWORD_NUM_16;
  default:
    vISA_ASSERT_UNREACHABLE("illegal Oword number.");
    return OWORD_NUM_ILLEGAL;
  }
}

VISA_Modifier Get_Common_ISA_SrcMod_From_G4_Mod(G4_SrcModifier mod) {
  switch (mod) {
  case Mod_src_undef:
    return MODIFIER_NONE;
  case Mod_Abs:
    return MODIFIER_ABS;
  case Mod_Minus:
    return MODIFIER_NEG;
  case Mod_Minus_Abs:
    return MODIFIER_NEG_ABS;
  case Mod_Not:
    return MODIFIER_NOT;
  default:
    vISA_ASSERT_UNREACHABLE("Wrong src modifier");
    return MODIFIER_NONE;
  }
}

VISA_Type getRawOperandType(const print_format_provider_t *header,
                            const raw_opnd &opnd) {
  unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();
  if (opnd.index < numPreDefinedVars) {
    // One of the pre-defined variables
    return getPredefinedVarType(mapExternalToInternalPreDefVar(opnd.index));
  }

  uint32_t opnd_index = opnd.index - numPreDefinedVars;
  const var_info_t *currVar = header->getVar(opnd_index);
  return currVar->getType();
}

VISA_Type getVectorOperandType(const print_format_provider_t *header,
                               const vector_opnd &opnd) {
  unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();
  switch (opnd.getOperandClass()) {
  case OPERAND_GENERAL:
    if (opnd.opnd_val.gen_opnd.index < numPreDefinedVars) {
      // One of the pre-defined variables
      return getPredefinedVarType(
          mapExternalToInternalPreDefVar(opnd.getOperandIndex()));
    } else {
      const var_info_t &var =
          *header->getVar(opnd.getOperandIndex() - numPreDefinedVars);
      return var.getType();
    }
  case OPERAND_ADDRESS:
    return ISA_TYPE_UW;
  case OPERAND_PREDICATE:
    return ISA_TYPE_BOOL;
  case OPERAND_INDIRECT:
    return (VISA_Type)(opnd.opnd_val.indirect_opnd.bit_property & 0xF);
  case OPERAND_ADDRESSOF:
    return ISA_TYPE_UW;
  case OPERAND_IMMEDIATE:
    return (VISA_Type)(opnd.opnd_val.const_opnd.type & 0xF);
  case OPERAND_STATE:
    return ISA_TYPE_UD;
  default:
    return ISA_TYPE_UD;
  }
}

const raw_opnd &getRawOperand(const CISA_INST *inst, unsigned i) {
  vISA_ASSERT(inst, "Argument Exception: argument inst is NULL.");
  vISA_ASSERT(inst->opnd_num > i,
               "No such operand, i, for instruction inst.");
  return inst->opnd_array[i]->_opnd.r_opnd;
}

bool isNullRawOperand(const CISA_INST *inst, unsigned i) {
  vISA_ASSERT(inst, "Argument Exception: argument inst is NULL.");
  vISA_ASSERT(inst->opnd_num > i,
               "No such operand, i, for instruction inst.");
  return inst->opnd_array[i]->_opnd.r_opnd.index == 0;
}

bool isNotNullRawOperand(const CISA_INST *inst, unsigned i) {
  return !isNullRawOperand(inst, i);
}

const vector_opnd &getVectorOperand(const CISA_INST *inst, unsigned i) {
  vISA_ASSERT(inst, "Argument Exception: argument inst is NULL.");
  vISA_ASSERT(inst->opnd_num > i,
               "No such operand, i, for instruction inst.");
  return inst->opnd_array[i]->_opnd.v_opnd;
}

CISA_opnd_type getOperandType(const CISA_INST *inst, unsigned i) {
  vISA_ASSERT(inst, "Argument Exception: argument inst is NULL.");
  vISA_ASSERT(inst->opnd_num > i,
               "No such operand, i, for instruction inst.");
  return inst->opnd_array[i]->opnd_type;
}

int64_t typecastVals(const void *value, VISA_Type isaType) {
  int64_t retVal = 0;
  switch (isaType) {
  case ISA_TYPE_UD:
  case ISA_TYPE_UV:
  case ISA_TYPE_VF: {
    retVal = (int64_t)(*((unsigned int *)value));
    break;
  }
  case ISA_TYPE_D:
  case ISA_TYPE_V: {
    retVal = (int64_t)(*((int *)value));
    break;
  }
  case ISA_TYPE_UW: {
    retVal = (int64_t)(*((uint16_t *)value));
    break;
  }
  case ISA_TYPE_W: {
    retVal = (int64_t)(*((int16_t *)value));
    break;
  }
  case ISA_TYPE_UB: {
    retVal = (int64_t)(*((uint8_t *)value));
    break;
  }
  case ISA_TYPE_B: {
    retVal = (int64_t)(*((int8_t *)value));
    break;
  }
  case ISA_TYPE_HF:
  case ISA_TYPE_BF: {
    // clear higher bits
    retVal = (int64_t)(*((uint16_t *)value));
    break;
  }

  default: {
    vISA_ASSERT_UNREACHABLE("invalid isa type");
    return -1;
  }
  }
  return retVal;
}

// convert binary vISA surface id to GEN surface index
int Get_PreDefined_Surf_Index(int index, TARGET_PLATFORM platform) {
  if (platform < GENX_SKL) {
    switch (index) {
    case 1:
      return PREDEF_SURF_1_OLD;
    case 2:
      return PREDEF_SURF_2_OLD;
    case 3:
      return PREDEF_SURF_3_OLD;
    default:;
      // fallthrough
    }
  }

  return vISAPreDefSurf[index].genId;
}

const char *createStringCopy(std::string_view name, vISA::Mem_Manager &m_mem) {
  if (name.size() == 0)
    return "";

  // TODO: look into relaxing this
  static const size_t MAX_VISA_BINARY_STRING_LENGTH = 256;

  size_t copyLen = name.size();
  if (copyLen >= MAX_VISA_BINARY_STRING_LENGTH) {
    copyLen = MAX_VISA_BINARY_STRING_LENGTH - 1;
  }
  char *copy = (char *)m_mem.alloc(copyLen + 1);
  strncpy_s(copy, copyLen + 1, name.data(), copyLen);
  return copy;
}

std::string sanitizeLabelString(std::string str) {
  auto isReservedChar = [](char c) {
    return !isalnum(c) && c != '_' && c != '$';
  };
  std::replace_if(str.begin(), str.end(), isReservedChar, '_');
  return str;
}

// This function scrubs out illegal file path characters
//
// NOTE: we must permit directory separators though since the string is a path
std::string sanitizePathString(std::string str) {
#ifdef _WIN32
  // better cross platform behavior ./foo/bar.asm => to backslashes
  auto isFwdSlash = [](char c) { return c == '/'; };
  std::replace_if(str.begin(), str.end(), isFwdSlash, '\\');
#endif

  auto isReservedChar = [](char c) {
#ifdef _WIN32
    // c.f.
    // https://docs.microsoft.com/en-us/windows/desktop/fileio/naming-a-file
    switch (c) {
    // we need these because we have a full path
    // case '\\': path separator
    case ':': // can be a drive suffix, but we handle this manually
    case '"':
    case '*':
    case '|':
    case '?':
    case '<':
    case '>':
      return true;
    default:
      return !isprint(c) && !isspace(c);
    }
    return false;
#else
    return c == ':' || (!isprint(c) && !isspace(c));
#endif
  };

#ifdef _WIN32
  if (str.length() > 2 && isalnum(str[0]) && str[1] == ':') {
    // drive prefix: D:... or D:
    std::replace_if(str.begin() + 2, str.end(), isReservedChar, '_');
  } else {
    std::replace_if(str.begin(), str.end(), isReservedChar, '_');
  }
#else
  std::replace_if(str.begin(), str.end(), isReservedChar, '_');
#endif
  return str;
}

const char *toString(GenPrecision P) {
  int ix = (int)P;
  if (ix > (int)GenPrecision::INVALID && ix < (int)GenPrecision::TOTAL_NUM) {
    return GenPrecisionTable[ix].Name;
  }
  return "?";
}

bool strEndsWith(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() &&
         0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

bool strStartsWith(const std::string &str, const std::string &prefix) {
  return str.size() >= prefix.size() &&
         0 == str.compare(0, prefix.size(), prefix);
}

LSC_CACHE_OPTS convertLSCLoadStoreCacheControlEnum(LSC_L1_L3_CC L1L3cc,
                                                   bool isLoad) {
  LSC_CACHE_OPTS cacheOpts{LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT};
  switch (L1L3cc) {
  case LSC_L1DEF_L3DEF:
    cacheOpts = {LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT};
    break;
  case LSC_L1UC_L3UC:
    cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED};
    break;
  case LSC_L1UC_L3C_WB:
    cacheOpts = {LSC_CACHING_UNCACHED,
                 isLoad ? LSC_CACHING_CACHED : LSC_CACHING_WRITEBACK};
    break;
  case LSC_L1C_WT_L3UC:
    cacheOpts = {isLoad ? LSC_CACHING_CACHED : LSC_CACHING_WRITETHROUGH,
                 LSC_CACHING_UNCACHED};
    break;
  case LSC_L1C_WT_L3C_WB:
    if (isLoad)
      cacheOpts = {LSC_CACHING_CACHED, LSC_CACHING_CACHED};
    else
      cacheOpts = {LSC_CACHING_WRITETHROUGH, LSC_CACHING_WRITEBACK};
    break;
  case LSC_L1S_L3UC:
    cacheOpts = {LSC_CACHING_STREAMING, LSC_CACHING_UNCACHED};
    break;
  case LSC_L1S_L3C_WB:
    cacheOpts = {LSC_CACHING_STREAMING,
                 isLoad ? LSC_CACHING_CACHED : LSC_CACHING_WRITEBACK};
    break;
  case LSC_L1IAR_WB_L3C_WB:
    if (isLoad)
      cacheOpts = {LSC_CACHING_READINVALIDATE, LSC_CACHING_CACHED};
    else
      cacheOpts = {LSC_CACHING_WRITEBACK, LSC_CACHING_WRITEBACK};
    break;
  case LSC_L1UC_L3CC:
    if (isLoad) {
      cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_CONSTCACHED};
      break;
    }
  case LSC_L1C_L3CC:
    if (isLoad) {
      cacheOpts = {LSC_CACHING_CACHED, LSC_CACHING_CONSTCACHED};
      break;
    }
  case LSC_L1IAR_L3IAR:
    if (isLoad) {
      cacheOpts = {LSC_CACHING_READINVALIDATE, LSC_CACHING_READINVALIDATE};
      break;
    }
  default:
    vISA_ASSERT_UNREACHABLE("unsupported caching option");
    break;
  }
  return cacheOpts;
}

void *vISA::allocCodeBlock(size_t sz) {
  // Just use vanilla malloc.
  // Alternative would be for FE compiler to provide a callback
  // function to perform allocation.
  return malloc(sz);
}

G4_Type getUnsignedType(unsigned short numByte) {
  switch (numByte) {
  case 1:
    return Type_UB;
  case 2:
    return Type_UW;
  case 4:
    return Type_UD;
  case 8:
    return Type_UQ;
  default:
    vISA_ASSERT_UNREACHABLE("illegal type width");
    return Type_UD;
  }
}

G4_Type getSignedType(unsigned short numByte) {
  switch (numByte) {
  case 1:
    return Type_B;
  case 2:
    return Type_W;
  case 4:
    return Type_D;
  case 8:
    return Type_Q;
  default:
    vISA_ASSERT_UNREACHABLE("illegal type width");
    return Type_D;
  }
}
