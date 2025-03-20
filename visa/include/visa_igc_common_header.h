/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _VISA_IGC_COMMON_HEADER_H_
#define _VISA_IGC_COMMON_HEADER_H_

#ifndef ARRAY_COUNT
#define ARRAY_COUNT( x ) ( sizeof( x ) / sizeof( x[ 0 ] ) )
#endif

typedef enum {
  VISA_SUCCESS = 0,
  VISA_FAILURE = -1,
  // User has requested vISA to early exit after a pass via the -stopafter
  // option.
  VISA_EARLY_EXIT = -2,
  VISA_SPILL = -3,
} vISAStatus;

typedef enum {
  vISA_DEFAULT,    // default mode: input is in-memory IR or vISA binary
  vISA_ASM_WRITER, // mode for inline asm: input is in-memory IR + inline vISA
                   // assembly, output is vISA text
  vISA_ASM_READER  // mode for visa text input
} vISABuilderMode;

// Target should be specified as follows
// - Kernel target attribute in VISA binarary or visaasm
typedef enum { VISA_CM = 0, VISA_3D = 1 } VISATarget;

typedef enum {
  PREDEFINED_NULL = 0,
  PREDEFINED_X = 1,
  PREDEFINED_Y = 2,
  PREDEFINED_GROUP_ID_X = 3,
  PREDEFINED_GROUP_ID_Y = 4,
  PREDEFINED_GROUP_ID_Z = 5,
  PREDEFINED_TSC = 6,
  PREDEFINED_R0 = 7,
  PREDEFINED_ARG = 8,
  PREDEFINED_RET = 9,
  PREDEFINED_FE_SP = 10,
  PREDEFINED_FE_FP = 11,
  PREDEFINED_HW_TID = 12,
  PREDEFINED_SR0 = 13,
  PREDEFINED_CR0 = 14,
  PREDEFINED_CE0 = 15,
  PREDEFINED_DBG = 16,
  PREDEFINED_COLOR = 17,
  PREDEFINED_IMPL_ARG_BUF_PTR = 18,
  PREDEFINED_LOCAL_ID_BUF_PTR = 19,
  PREDEFINED_MSG0 = 20,
  PREDEFINED_VAR_INVALID
} PreDefined_Vars;

typedef enum {
  // FIXME: why not expose PREDEF_SURF_0 etc. directly?
  PREDEFINED_SURFACE_SLM = 0,
  PREDEFINED_SURFACE_STACK = 1,
  PREDEFINED_SURFACE_SCRATCH = 3,
  PREDEFINED_SURFACE_T252 = 4,
  PREDEFINED_SURFACE_T255 = 5,
  PREDEFINED_SURFACE_INVALID
} PreDefined_Surface;

typedef enum {
  ATOMIC_ADD = 0x0,
  ATOMIC_SUB = 0x1,
  ATOMIC_INC = 0x2,
  ATOMIC_DEC = 0x3,
  ATOMIC_MIN = 0x4,
  ATOMIC_MAX = 0x5,
  ATOMIC_XCHG = 0x6,
  ATOMIC_CMPXCHG = 0x7,
  ATOMIC_AND = 0x8,
  ATOMIC_OR = 0x9,
  ATOMIC_XOR = 0xa,
  ATOMIC_IMIN = 0xb,
  ATOMIC_IMAX = 0xc,
  ATOMIC_PREDEC = 0xd,
  ATOMIC_FMAX = 0x10,
  ATOMIC_FMIN = 0x11,
  ATOMIC_FCMPWR = 0x12,
  ATOMIC_FADD = 0x13,
  ATOMIC_FSUB = 0x14,
  ATOMIC_UNDEF
} VISAAtomicOps;

/*
 * Various enumerations representing the binary encoding definitions for the
 * common ISA
 *
 */
typedef enum : unsigned char {
  ISA_TYPE_UD = 0x0,
  ISA_TYPE_D = 0x1,
  ISA_TYPE_UW = 0x2,
  ISA_TYPE_W = 0x3,
  ISA_TYPE_UB = 0x4,
  ISA_TYPE_B = 0x5,
  ISA_TYPE_DF = 0x6,
  ISA_TYPE_F = 0x7,
  ISA_TYPE_V = 0x8,
  ISA_TYPE_VF = 0x9,
  ISA_TYPE_BOOL = 0xA,
  ISA_TYPE_UQ = 0xB,
  ISA_TYPE_UV = 0xC,
  ISA_TYPE_Q = 0xD,
  ISA_TYPE_HF = 0xE,
  ISA_TYPE_BF = 0xF,
  ISA_TYPE_NUM
} VISA_Type;

typedef struct {
  VISA_Type CISAType;
  const char *typeName;
  int typeSize;
} CISATypeInfo;

extern CISATypeInfo CISATypeTable[ISA_TYPE_NUM];

typedef enum {
  ALIGN_UNDEF = 0x0,
  ALIGN_BYTE = 0x0,
  ALIGN_WORD = 0x1,
  ALIGN_DWORD = 0x2,
  ALIGN_QWORD = 0x3,
  ALIGN_OWORD = 0x4,
  ALIGN_GRF = 0x5,
  ALIGN_2_GRF = 0x6,
  ALIGN_HWORD = 0x7,
  ALIGN_32WORD = 0x8,
  ALIGN_64WORD = 0x9,

  ALIGN_TOTAL_NUM = 0xA
} VISA_Align;

typedef enum {
  LABEL_BLOCK = 0x0,
  LABEL_SUBROUTINE = 0x1,
  LABEL_FC = 0x2,
  LABEL_FUNCTION = 0x3,
  LABEL_DIVERGENT_RESOURCE_LOOP = 0x4,
} VISA_Label_Kind;

typedef enum {
  ISA_CMP_E = 0, /* Zero or Equal */
  ISA_CMP_NE,    /* Not Zero or Not Equal */
  ISA_CMP_G,     /* Greater-than */
  ISA_CMP_GE,    /* Greater-than-or-equal */
  ISA_CMP_L,     /* Less-than */
  ISA_CMP_LE,    /* Less-than-or-equal */
  ISA_CMP_UNDEF
} VISA_Cond_Mod;

typedef enum {
  MODIFIER_NONE = 0x0,
  MODIFIER_ABS = 0x1,
  MODIFIER_NEG = 0x2,
  MODIFIER_NEG_ABS = 0x3,
  MODIFIER_SAT = 0x4,
  MODIFIER_NOT = 0x5 // BDW only
} VISA_Modifier;

typedef enum {
  PredState_NO_INVERSE = 0,
  PredState_INVERSE = 1
} VISA_PREDICATE_STATE;

typedef enum {
  EXEC_SIZE_1 = 0x0,
  EXEC_SIZE_2 = 0x1,
  EXEC_SIZE_4 = 0x2,
  EXEC_SIZE_8 = 0x3,
  EXEC_SIZE_16 = 0x4,
  EXEC_SIZE_32 = 0x5,
  EXEC_SIZE_ILLEGAL = 0x6
} VISA_Exec_Size;

// predicate control
// Currently only support Any and all
typedef enum {
  PRED_CTRL_NON = 0x0,
  PRED_CTRL_ANY = 0x1,
  PRED_CTRL_ALL = 0x2,
  PRED_CTRL_UNDEF
} VISA_PREDICATE_CONTROL;

typedef enum {
  vISA_EMASK_M1 = 0,
  vISA_EMASK_M2 = 1,
  vISA_EMASK_M3 = 2,
  vISA_EMASK_M4 = 3,
  vISA_EMASK_M5 = 4,
  vISA_EMASK_M6 = 5,
  vISA_EMASK_M7 = 6,
  vISA_EMASK_M8 = 7,
  vISA_EMASK_M1_NM = 8,
  vISA_EMASK_M2_NM = 9,
  vISA_EMASK_M3_NM = 10,
  vISA_EMASK_M4_NM = 11,
  vISA_EMASK_M5_NM = 12,
  vISA_EMASK_M6_NM = 13,
  vISA_EMASK_M7_NM = 14,
  vISA_EMASK_M8_NM = 15,
  vISA_NUM_EMASK = 16
} VISA_EMask_Ctrl;

typedef enum {
  OWORD_NUM_1 = 0x0,
  OWORD_NUM_2 = 0x1,
  OWORD_NUM_4 = 0x2,
  OWORD_NUM_8 = 0x3,
  OWORD_NUM_16 = 0x4,
  OWORD_NUM_ILLEGAL = 0x5
} VISA_Oword_Num;

// media load inst modifiers
typedef enum {
  MEDIA_LD_nomod = 0x0,
  MEDIA_LD_modified = 0x1,
  MEDIA_LD_top = 0x2,
  MEDIA_LD_bottom = 0x3,
  MEDIA_LD_top_mod = 0x4,
  MEDIA_LD_bottom_mod = 0x5,
  MEDIA_LD_Mod_NUM
} MEDIA_LD_mod;

typedef enum {
  VME_OP_MODE_INTER = 0x0,
  VME_OP_MODE_INTRA = 0x1,
  VME_OP_MODE_ALL = 0x2,
  VME_OP_MODE_NUM = 0x3
} COMMON_ISA_VME_OP_MODE;

typedef enum {
  VME_STREAM_DISABLE = 0,
  VME_STREAM_OUT = 1,
  VME_STREAM_IN = 2,
  VME_STREAM_IN_OUT = 3
} COMMON_ISA_VME_STREAM_MODE;

typedef enum {
  VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START = 0,
  VME_SEARCH_SINGLE_REF_SINGLE_REC_DUAL_START = 1,
  VME_SEARCH_SINGLE_REF_DUAL_REC = 3,
  VME_SEARCH_DUAL_REF_DUAL_REC = 7
} COMMON_ISA_VME_SEARCH_CTRL;

typedef enum { CISA_DM_FMIN = 0, CISA_DM_FMAX = 1 } CISA_MIN_MAX_SUB_OPCODE;

typedef enum {
  CISA_PLANE_Y = 0,
  CISA_PLANE_UV = 1,
  CISA_PLANE_V = 2
} CISA_PLANE_ID;

typedef enum {
  /* 0x0 reserved -- used for illegal/nop op? */
  ISA_RESERVED_0 = 0x0,
  ISA_ADD = 0x1,
  ISA_AVG = 0x2,
  ISA_DIV = 0x3,
  ISA_DP2 = 0x4,
  ISA_DP3 = 0x5,
  ISA_DP4 = 0x6,
  ISA_DPH = 0x7,
  ISA_EXP = 0x8,
  ISA_FRC = 0x9,
  ISA_LINE = 0xA,
  ISA_LOG = 0xB,
  ISA_MAD = 0xC,
  ISA_MULH = 0xD,
  ISA_LRP = 0xE,
  ISA_MOD = 0xF,
  ISA_MUL = 0x10,
  ISA_POW = 0x11,
  ISA_RNDD = 0x12,
  ISA_RNDU = 0x13,
  ISA_RNDE = 0x14,
  ISA_RNDZ = 0x15,
  ISA_SAD2 = 0x16,
  ISA_SIN = 0x17,
  ISA_COS = 0x18,
  ISA_SQRT = 0x19,
  ISA_RSQRT = 0x1A,
  ISA_INV = 0x1B,
  ISA_DPASW = 0x1C,
  ISA_FCVT = 0x1D,
  ISA_QF_CVT = ISA_FCVT, // temp for cmc
  ISA_SRND = 0x1E,
  ISA_LZD = 0x1F,
  ISA_AND = 0x20,
  ISA_OR = 0x21,
  ISA_XOR = 0x22,
  ISA_NOT = 0x23,
  ISA_SHL = 0x24,
  ISA_SHR = 0x25,
  ISA_ASR = 0x26,
  ISA_CBIT = 0x27,
  ISA_ADDR_ADD = 0x28,
  ISA_MOV = 0x29,
  ISA_SEL = 0x2A,
  ISA_SETP = 0x2B,
  ISA_CMP = 0x2C,
  ISA_MOVS = 0x2D,
  ISA_FBL = 0x2E,
  ISA_FBH = 0x2F,
  ISA_SUBROUTINE = 0x30,
  ISA_LABEL = 0x31,
  ISA_JMP = 0x32,
  ISA_CALL = 0x33,
  ISA_RET = 0x34,
  ISA_OWORD_LD = 0x35,
  ISA_OWORD_ST = 0x36,
  ISA_MEDIA_LD = 0x37,
  ISA_MEDIA_ST = 0x38,
  ISA_GATHER = 0x39,
  ISA_SCATTER = 0x3A,
  ISA_RESERVED_3B = 0x3B,
  ISA_OWORD_LD_UNALIGNED = 0x3C,
  ISA_RESERVED_3D = 0x3D,
  ISA_RESERVED_3E = 0x3E,
  ISA_RESERVED_3F = 0x3F,
  ISA_SAMPLE = 0x40,
  ISA_SAMPLE_UNORM = 0x41,
  ISA_LOAD = 0x42,
  ISA_AVS = 0x43,
  ISA_VA = 0x44,
  ISA_FMINMAX = 0x45,
  ISA_BFE = 0x46,
  ISA_BFI = 0x47,
  ISA_BFREV = 0x48,
  ISA_ADDC = 0x49,
  ISA_SUBB = 0x4A,
  ISA_GATHER4_TYPED = 0x4B,
  ISA_SCATTER4_TYPED = 0x4C,
  ISA_VA_SKL_PLUS = 0x4D,
  ISA_SVM = 0x4E,
  ISA_IFCALL = 0x4F,
  ISA_FADDR = 0x50,
  ISA_FILE = 0x51,
  ISA_LOC = 0x52,
  ISA_RESERVED_53 = 0x53,
  ISA_VME_IME = 0x54,
  ISA_VME_SIC = 0x55,
  ISA_VME_FBR = 0x56,
  ISA_VME_IDM = 0x57,
  ISA_RESERVED_58 = 0x58,
  ISA_BARRIER = 0x59,
  ISA_SAMPLR_CACHE_FLUSH = 0x5A,
  ISA_WAIT = 0x5B,
  ISA_FENCE = 0x5C,
  ISA_RAW_SEND = 0x5D,
  ISA_RESERVED_5E = 0x5E,
  ISA_YIELD = 0x5F,
  ISA_NBARRIER = 0x60,
  ISA_RESERVED_61 = 0x61,
  ISA_RESERVED_62 = 0x62,
  ISA_RESERVED_63 = 0x63,
  ISA_RESERVED_64 = 0x64,
  ISA_RESERVED_65 = 0x65,
  ISA_RESERVED_66 = 0x66,
  ISA_FCALL = 0x67,
  ISA_FRET = 0x68,
  ISA_SWITCHJMP = 0x69,
  ISA_SAD2ADD = 0x6A,
  ISA_PLANE = 0x6B,
  ISA_GOTO = 0x6C,
  ISA_3D_SAMPLE = 0x6D,
  ISA_3D_LOAD = 0x6E,
  ISA_3D_GATHER4 = 0x6F,
  ISA_3D_INFO = 0x70,
  ISA_3D_RT_WRITE = 0x71,
  ISA_3D_URB_WRITE = 0x72,
  ISA_3D_TYPED_ATOMIC = 0x73,
  ISA_GATHER4_SCALED = 0x74,
  ISA_SCATTER4_SCALED = 0x75,
  ISA_RESERVED_76 = 0x76,
  ISA_RESERVED_77 = 0x77,
  ISA_GATHER_SCALED = 0x78,
  ISA_SCATTER_SCALED = 0x79,
  ISA_RAW_SENDS = 0x7A,
  ISA_LIFETIME = 0x7B,
  ISA_SBARRIER = 0x7C,
  ISA_DWORD_ATOMIC = 0x7D,
  ISA_SQRTM = 0x7E,
  ISA_DIVM = 0x7F,
  ISA_ROL = 0x80,
  ISA_ROR = 0x81,
  ISA_DP4A = 0x82,
  ISA_DPAS = 0x83,
  ISA_ADD3 = 0x84,
  ISA_BFN = 0x85,
  ISA_QW_GATHER = 0x86,
  ISA_QW_SCATTER = 0x87,
  ISA_RESERVED_88 = 0x88,
  ISA_LSC_UNTYPED = 0x89,
  ISA_LSC_TYPED = 0x8A,
  ISA_LSC_FENCE = 0x8B,
  ISA_RESERVED_8C = 0x8C,
  ISA_RESERVED_8D = 0x8D,
  ISA_RESERVED_8E = 0x8E,
  ISA_RESERVED_8F = 0x8F,
  ISA_RESERVED_90 = 0x90,
  ISA_MADW = 0x91,
  ISA_ADD3O = 0x92,
  ISA_RESERVED_93 = 0x93,
  ISA_BREAKPOINT = 0x94,
  ISA_RESERVED_95 = 0x95,
  ISA_RESERVED_96 = 0x96,
  ISA_RESERVED_97 = 0x97,
  ISA_RESERVED_98 = 0x98,
  ISA_RESERVED_99 = 0x99,
  ISA_RESERVED_9A = 0x9A,
  ISA_INVM = 0x9B,
  ISA_RSQTM = 0x9C,
  ISA_NUM_OPCODE,
  ISA_OPCODE_ENUM_SIZE = 0xFF
} ISA_Opcode;

typedef enum {
  VISA_3D_SAMPLE = 0,
  VISA_3D_SAMPLE_B = 1,
  VISA_3D_SAMPLE_L = 2,
  VISA_3D_SAMPLE_C = 3,
  VISA_3D_SAMPLE_D = 4,
  VISA_3D_SAMPLE_B_C = 5,
  VISA_3D_SAMPLE_L_C = 6,
  VISA_3D_LD = 7,
  VISA_3D_GATHER4 = 8,
  VISA_3D_LOD = 9,
  VISA_3D_RESINFO = 10,
  VISA_3D_SAMPLEINFO = 11,
  VISA_3D_SAMPLE_KILLPIX = 12,
  VISA_3D_GATHER4_L = 13,
  VISA_3D_GATHER4_B = 14,
  VISA_3D_GATHER4_I = 15,
  VISA_3D_GATHER4_C = 16,
  VISA_3D_GATHER4_PO = 17,
  VISA_3D_GATHER4_PO_C = 18,
  // Note: Xe2 reuses GATHER4_PO* opcode values (Xe2 doesn't have GATHER4_PO*)
  VISA_3D_SAMPLE_D_C_MLOD = 17,
  VISA_3D_SAMPLE_MLOD = 18,
  VISA_3D_SAMPLE_C_MLOD = 19,
  VISA_3D_SAMPLE_D_C = 20,
  VISA_3D_GATHER4_I_C = 21,
  VISA_3D_GATHER4_L_C = 23,
  VISA_3D_SAMPLE_LZ = 24,
  VISA_3D_SAMPLE_C_LZ = 25,
  VISA_3D_LD_LZ = 26,
  VISA_3D_LD_L = 27,
  VISA_3D_LD2DMS_W = 28,
  VISA_3D_LD_MCS = 29,
// positional offsets
  VISA_3D_SAMPLE_PO = 32,
  VISA_3D_SAMPLE_PO_B = 33,
  VISA_3D_SAMPLE_PO_L = 34,
  VISA_3D_SAMPLE_PO_C = 35,
  VISA_3D_SAMPLE_PO_D = 36,
  VISA_3D_SAMPLE_PO_L_C = 38,
  VISA_3D_GATHER4_PO_PACKED = 40,
  VISA_3D_GATHER4_PO_PACKED_L = 45,
  VISA_3D_GATHER4_PO_PACKED_B = 46,
  VISA_3D_GATHER4_PO_PACKED_I = 47,
  VISA_3D_GATHER4_PO_PACKED_C = 48,
  VISA_3D_GATHER4_PO_PACKED_I_C = 53,
  VISA_3D_GATHER4_PO_PACKED_L_C = 55,
  VISA_3D_SAMPLE_PO_LZ = 56,
  VISA_3D_SAMPLE_PO_C_LZ = 57,
  VISA_3D_TOTAL_NUM_OPS,
} VISASampler3DSubOpCode;

typedef enum {
  CHANNEL_MASK_NOMASK = 0x0,
  CHANNEL_MASK_R = 0x1,
  CHANNEL_MASK_G = 0x2,
  CHANNEL_MASK_RG = 0x3,
  CHANNEL_MASK_B = 0x4,
  CHANNEL_MASK_RB = 0x5,
  CHANNEL_MASK_GB = 0x6,
  CHANNEL_MASK_RGB = 0x7,
  CHANNEL_MASK_A = 0x8,
  CHANNEL_MASK_RA = 0x9,
  CHANNEL_MASK_GA = 0xa,
  CHANNEL_MASK_RGA = 0xb,
  CHANNEL_MASK_BA = 0xc,
  CHANNEL_MASK_RBA = 0xd,
  CHANNEL_MASK_GBA = 0xe,
  CHANNEL_MASK_RGBA = 0xf,
  CHANNEL_MASK_NUM
} VISAChannelMask;

typedef enum {
  CHANNEL_16_BIT_FULL = 0,
  CHANNEL_16_BIT_DOWNSAMPLED = 1,
  CHANNEL_8_BIT_FULL = 2,
  CHANNEL_8_BIT_DOWNSAMPLED = 3,
  CHANNEL_OUTPUT_NUM = 4
} CHANNEL_OUTPUT_FORMAT;

typedef enum {
  VISA_3D_GATHER4_CHANNEL_R = 0,
  VISA_3D_GATHER4_CHANNEL_G = 1,
  VISA_3D_GATHER4_CHANNEL_B = 2,
  VISA_3D_GATHER4_CHANNEL_A = 3
} VISASourceSingleChannel;

typedef enum {
  GENX_NONE = -1,
  GENX_BDW,
  GENX_CHV,
  GENX_SKL,
  GENX_BXT,
  GENX_CNL,
  GENX_ICLLP,
  GENX_TGLLP,
  Xe_XeHPSDV,
  Xe_DG2,
  Xe_MTL,
  Xe_ARL,
  Xe_PVC,
  Xe_PVCXT,
  Xe2,
  Xe3,
  ALL
} TARGET_PLATFORM;

// gather/scatter element size
typedef enum {
  GATHER_SCATTER_BYTE = 0x0,
  GATHER_SCATTER_WORD = 0x1,
  GATHER_SCATTER_DWORD = 0x2,
  GATHER_SCATTER_BYTE_UNDEF
} GATHER_SCATTER_ELEMENT_SIZE;

/// Sampler8x8 DevBDW+ Functionality Opcodes
typedef enum VA_fopcode {
  AVS_FOPCODE = 0x00,
  Convolve_FOPCODE = 0x01,
  MINMAX_FOPCODE = 0x02,
  MINMAXFILTER_FOPCODE = 0x03,
  ERODE_FOPCODE = 0x04,
  Dilate_FOPCODE = 0x05,
  BoolCentroid_FOPCODE = 0x06,
  Centroid_FOPCODE = 0x07,
  VA_OP_CODE_1D_CONVOLVE_VERTICAL = 0x08,
  VA_OP_CODE_1D_CONVOLVE_HORIZONTAL = 0x09,
  VA_OP_CODE_1PIXEL_CONVOLVE = 0x0A,
  VA_OP_CODE_FLOOD_FILL = 0x0B,
  VA_OP_CODE_LBP_CREATION = 0x0C,
  VA_OP_CODE_LBP_CORRELATION = 0x0D,
  VA_OP_CODE_NONE = 0x0E,
  VA_OP_CODE_CORRELATION_SEARCH = 0x0F,
  ISA_HDC_CONV = 0x10,
  ISA_HDC_MMF = 0x11,
  ISA_HDC_ERODE = 0x12,
  ISA_HDC_DILATE = 0x13,
  ISA_HDC_LBPCORRELATION = 0x14,
  ISA_HDC_LBPCREATION = 0x15,
  ISA_HDC_1DCONV_H = 0x16,
  ISA_HDC_1DCONV_V = 0x17,
  ISA_HDC_1PIXELCONV = 0x18,
  VA_OP_CODE_UNDEFINED = 0x19
} ISA_VA_Sub_Opcode;

typedef enum _OutputFormatControl_ {
  AVS_16_FULL = 0,
  AVS_16_DOWN_SAMPLE = 1,
  AVS_8_FULL = 2,
  AVS_8_DOWN_SAMPLE = 3
} OutputFormatControl;

typedef enum _AVSExecMode_ {
  AVS_16x4 = 0,
  AVS_8x4 = 1,
  AVS_16x8 = 2,
  AVS_4x4 = 3
} AVSExecMode;

typedef enum _MMFExecMode_ {
  VA_MMF_16x4 = 0,
  VA_MMF_16x1 = 2,
  VA_MMF_1x1 = 3
} MMFExecMode;

typedef enum _MMFEnableMode_ {
  VA_MINMAX_ENABLE = 0,
  VA_MAX_ENABLE = 1,
  VA_MIN_ENABLE = 2
} MMFEnableMode;

typedef enum _CONVExecMode_ { VA_CONV_16x4 = 0, VA_CONV_16x1 = 2 } CONVExecMode;

typedef enum _EDExecMode_ {
  VA_ED_64x4 = 0,
  VA_ED_32x4 = 1,
  VA_ED_64x1 = 2,
  VA_ED_32x1 = 3
} EDExecMode;

typedef enum _EDMode_ { VA_ERODE = 4, VA_DILATE = 5 } EDMode;

typedef enum _LBPCreationMode_ {
  VA_3x3_AND_5x5 = 0,
  VA_3x3 = 1,
  VA_5x5 = 2
} LBPCreationMode;

typedef enum _Convolve1DDirection_ {
  VA_H_DIRECTION = 0,
  VA_V_DIRECTION = 1
} Convolve1DDirection;

typedef enum _CONV1PixelExecMode_ {
  VA_CONV1P_16x4 = 0,
  VA_CONV1P_16x1 = 2,
  VA_CONV1P_1x1 = 3
} CONV1PixelExecMode;

typedef enum _HDCReturnFormat_ {
  VA_HDC_CONVOVLE_Y16 = 0,
  VA_HDC_CONVOLVE_Y8 = 1
} HDCReturnFormat;

typedef enum _CONVHDCRegionSize_ {
  VA_HDC_CONVOLVE_15x15 = 0,
  VA_HDC_CONVOLVE_31x31 = 1
} CONVHDCRegionSize;

typedef enum {
  SVM_BLOCK_NUM_1 = 0x0,
  SVM_BLOCK_NUM_2 = 0x1,
  SVM_BLOCK_NUM_4 = 0x2,
  SVM_BLOCK_NUM_8 = 0x3
} VISA_SVM_Block_Num;

typedef enum {
  SVM_BLOCK_TYPE_BYTE = 0x0,
  SVM_BLOCK_TYPE_DWORD = 0x1,
  SVM_BLOCK_TYPE_QWORD = 0x2
} VISA_SVM_Block_Type;

typedef struct _vISA_RT_CONTROLS {
  unsigned s0aPresent : 1;     // src0 Alpha
  unsigned oMPresent : 1;      // oMask
  unsigned zPresent : 1;       // depth
  unsigned RTIndexPresent : 1; // Whether need to set RTIndex in header
  unsigned
      isLastWrite : 1; // is last RT Write, sets Last Render Target Select bit
  unsigned isPerSample : 1; // Enables Per Sample Render Target Write
  unsigned isStencil : 1;
  unsigned isCoarseMode : 1;  // controls coasrse mode bit inmsg descriptor
  unsigned isSampleIndex : 1; // controls whether sampleIndex is used.
  unsigned isHeaderMaskfromCe0 : 1;
  unsigned isNullRT : 1; // null render target
} vISA_RT_CONTROLS;

typedef enum { LIFETIME_START = 0, LIFETIME_END = 1 } VISAVarLifetime;

enum class GenPrecision : unsigned char {
  INVALID = 0,

  U1 = 1,
  S1 = 2,
  U2 = 3,
  S2 = 4,
  U4 = 5,
  S4 = 6,
  U8 = 7,
  S8 = 8,
  BF16 = 9,  // bfloat16 (1, 8, 7)
  FP16 = 10, // half (1, 5, 10)
  BF8 = 11,  // bfloat8 (1, 5, 2)
  TF32 = 12, // TensorFloat (1, 8, 10), 19 bits
  HF8 = 14, // HF8 (1, 4, 3)
  TOTAL_NUM
};

///////////////////////////////////////////////////////////////////////////////
// Data types to support LSC load/store messages.
//

// The size of each data element
enum LSC_DATA_SIZE {
  LSC_DATA_SIZE_INVALID,
  LSC_DATA_SIZE_8b,  // DATA:u8...
  LSC_DATA_SIZE_16b, // DATA:u16...
  LSC_DATA_SIZE_32b, // DATA:u32...
  LSC_DATA_SIZE_64b, // DATA:u64...
                     // data types supporting conversion on load
// 8c32b reads load (8) bits, (c)onvert to (32) bits (zero extending)
// store truncates
//
// In DG2 and PVC the upper bits are undefined.
// XE2+ makes them zeros.
  LSC_DATA_SIZE_8c32b,   // DATA:u8c32...   (zero-extend / truncate)
  LSC_DATA_SIZE_16c32b,  // DATA:u16c32..   (zero-extend / truncate)
  LSC_DATA_SIZE_16c32bH, // DATA:u16c32h..  h means load to (h)igh 16
                         // data stored in upper 16; zero-fills bottom 16
                         // (bfloat raw conversion to 32b float)
};

// The number of elements per address ("vector" size)
enum LSC_DATA_ELEMS {
  LSC_DATA_ELEMS_INVALID,
  LSC_DATA_ELEMS_1,  // DATA:..x1
  LSC_DATA_ELEMS_2,  // DATA:..x2
  LSC_DATA_ELEMS_3,  // DATA:..x3
  LSC_DATA_ELEMS_4,  // DATA:..x4
  LSC_DATA_ELEMS_8,  // DATA:..x8
  LSC_DATA_ELEMS_16, // DATA:..x16
  LSC_DATA_ELEMS_32, // DATA:..x32
  LSC_DATA_ELEMS_64, // DATA:..x64
};

enum LSC_DATA_ORDER {
  LSC_DATA_ORDER_INVALID,
  LSC_DATA_ORDER_NONTRANSPOSE,
  LSC_DATA_ORDER_TRANSPOSE, // DATA:...t
};

enum LSC_DATA_CHMASK {
  LSC_DATA_CHMASK_INVALID,
  LSC_DATA_CHMASK_X = 1 << 0,
  LSC_DATA_CHMASK_Y = 1 << 1,
  LSC_DATA_CHMASK_Z = 1 << 2,
  LSC_DATA_CHMASK_W = 1 << 3,
};

struct LSC_DATA_SHAPE {
  LSC_DATA_SIZE size;
  LSC_DATA_ORDER order;
  union {
    LSC_DATA_ELEMS elems; // all other operations use the regular vector
    int chmask;           // for LSC_*_QUAD; bitmask of LSC_DATA_CHMASK
  };
};

struct LSC_DATA_SHAPE_BLOCK2D {
  LSC_DATA_SIZE size;
  LSC_DATA_ORDER order;
  int blocks; // the count of 2d blocks to load (array len)
  int width;  // the width (in elems) of the 2d region
  int height; // the height (in elems) of the 2d region
  bool vnni;  // perform a vnni transform on load
};
static const unsigned LSC_BLOCK2D_ADDR_PARAMS = 6;
struct LSC_DATA_SHAPE_TYPED_BLOCK2D {
  int width;  // the width (in bytes) of the 2d region
  int height; // the height (in elems) of the 2d region
};

enum LSC_ADDR_SIZE {
  LSC_ADDR_SIZE_INVALID,
  LSC_ADDR_SIZE_16b,  // [ADDR]:a16
  LSC_ADDR_SIZE_32b,  // [ADDR]:a32
  LSC_ADDR_SIZE_64b,  // [ADDR]:a64
};

enum LSC_ADDR_TYPE {
  LSC_ADDR_TYPE_INVALID,
  LSC_ADDR_TYPE_FLAT, // aka "stateless"
  LSC_ADDR_TYPE_BSS,  // bindless surface state offset
  LSC_ADDR_TYPE_SS,   // surface state offset
  LSC_ADDR_TYPE_BTI,  // binding table interface (legacy)
  //
  LSC_ADDR_TYPE_ARG, // pseudo address type for kernel arguments
};

enum class LSC_DOC_ADDR_SPACE {
  INVALID,
  PRIVATE,
  GLOBAL,
  LOCAL,
  GENERIC,
  RAYSTACK,
};

//
// Caching override behavior
//
// We support all combinations in IR, though not all are supported in hardware
// https://gfxspecs.intel.com/Predator/Home/Index/53560
typedef enum {
  LSC_CACHING_DEFAULT,        // .df
  LSC_CACHING_UNCACHED,       // .uc
  LSC_CACHING_CACHED,         // .ca
  LSC_CACHING_WRITEBACK,      // .wb
  LSC_CACHING_WRITETHROUGH,   // .wt
  LSC_CACHING_STREAMING,      // .st
  LSC_CACHING_READINVALIDATE, // .ri last use / invalidate after read
  LSC_CACHING_CONSTCACHED, // .cc
} LSC_CACHE_OPT;
// Only some combinations are legal (per platform)
struct LSC_CACHE_OPTS {
  LSC_CACHE_OPT l1;
  LSC_CACHE_OPT l3;

 LSC_CACHE_OPTS() = default;

 LSC_CACHE_OPTS(LSC_CACHE_OPT _l1, LSC_CACHE_OPT _l3)
   : l1(_l1),
     l3(_l3)
  { }

};

// L1,L3 available cache policies combinations
// Auxiliary enums for cache options translation from intrinsics into
// vISA representation.
typedef enum {
#define LSC_CACHE_CTRL_OPTION(Name, Val, Description) Name = Val,
#include "IGC/common/igc_regkeys_enums_defs.h"
  LSC_CACHE_CTRL_OPTIONS
#undef LSC_CACHE_CTRL_OPTION
#undef LSC_CACHE_CTRL_OPTIONS
} LSC_L1_L3_CC;

// Groups all the necessary address into a product type
struct LSC_ADDR {
  // The address model being used (e.g. flat, bti, bss, ss)
  LSC_ADDR_TYPE type;

  // An optional uniform immediate scale.  Default this to 1 if
  // scaling is not desired (or supported).  0 is an illegal value here.
  int immScale;

  // An optional uniform immediate offset; for the given address model,
  // if hardware supports this and the value is small enough to fit in the
  // descriptor, then it can be fused into the operation.  Conversely, if the
  // immediate value is too big or immediate offsets are not supported for
  // the given address type or device, codegen will generate separate adds
  // to the address.
  //
  // This value is intended to be pre-scaled and 'immScale'
  // doesn't impact it. I.e. the effective address is
  //   EA = immScale * ADDR + immOffset (result in bytes)
  int immOffset; // [...+0x100] (can be 0)

  // The number of bits per address; not all address models support all sizes
  LSC_ADDR_SIZE size; // e.g. :a64, :a32, ...

  // e.g private, global, local, generic, raystack
  // used for documentation in comments
  LSC_DOC_ADDR_SPACE addrSpace;
};

// The specific fence op
enum LSC_FENCE_OP {
  LSC_FENCE_OP_NONE,       // .none
  LSC_FENCE_OP_EVICT,      // .evict (dirty lines evicted and invalided;
                           // clean lines are invalidated)
  LSC_FENCE_OP_INVALIDATE, // .invalidate (inv. all clean lines; don't evict)
  LSC_FENCE_OP_DISCARD,    // .discard (dirty and clean lines written,
                           // but stay in cache)
  LSC_FENCE_OP_CLEAN,      // .clean
  LSC_FENCE_OP_FLUSHL3,    // .flushl3 (flush L3 only, but not L1)
  LSC_FENCE_OP_TYPE6 // .flushtype6
};

// The scope of a given IO operation (typically fence)
enum LSC_SCOPE {
  LSC_SCOPE_GROUP,  // .group  (thread group)
  LSC_SCOPE_LOCAL,  // .local (dss?)
  LSC_SCOPE_TILE,   // .tile
  LSC_SCOPE_GPU,    // .gpu
  LSC_SCOPE_GPUS,   // .gpus
  LSC_SCOPE_SYSREL, // .sysrel
  LSC_SCOPE_SYSACQ, // .sysacq
};

// The subset of SFIDs that are permitted for LSC messages
enum LSC_SFID {
  LSC_UGM,  // .ugm
  LSC_UGML, // .ugml
  LSC_TGM,  // .tgm
  LSC_SLM,  // .slm
  LSC_URB,  // .urb
};

enum LSC_OP {
  LSC_LOAD = 0x00,
  LSC_LOAD_STRIDED = 0x01, // aka "load_block"
  LSC_LOAD_QUAD = 0x02,    // aka "load_cmask"
  LSC_LOAD_BLOCK2D = 0x03,
  LSC_STORE = 0x04,
  LSC_STORE_STRIDED = 0x05, // aka "load_block"
  LSC_STORE_QUAD = 0x06,    // aka "store_cmask"
  LSC_STORE_BLOCK2D = 0x07,
  //
  LSC_ATOMIC_IINC = 0x08,
  LSC_ATOMIC_IDEC = 0x09,
  LSC_ATOMIC_LOAD = 0x0A,
  LSC_ATOMIC_STORE = 0x0B,
  LSC_ATOMIC_IADD = 0x0C,
  LSC_ATOMIC_ISUB = 0x0D,
  LSC_ATOMIC_SMIN = 0x0E,
  LSC_ATOMIC_SMAX = 0x0F,
  LSC_ATOMIC_UMIN = 0x10,
  LSC_ATOMIC_UMAX = 0x11,
  LSC_ATOMIC_ICAS = 0x12,
  LSC_ATOMIC_FADD = 0x13,
  LSC_ATOMIC_FSUB = 0x14,
  LSC_ATOMIC_FMIN = 0x15,
  LSC_ATOMIC_FMAX = 0x16,
  LSC_ATOMIC_FCAS = 0x17,
  LSC_ATOMIC_AND = 0x18,
  LSC_ATOMIC_OR = 0x19,
  LSC_ATOMIC_XOR = 0x1A,
  //
  LSC_LOAD_STATUS = 0x1B,
  LSC_STORE_UNCOMPRESSED = 0x1C,
  LSC_CCS_UPDATE = 0x1D,
  LSC_READ_STATE_INFO = 0x1E,
  LSC_FENCE = 0x1F,
//
  LSC_APNDCTR_ATOMIC_ADD = 0x28,
  LSC_APNDCTR_ATOMIC_SUB = 0x29,
  LSC_APNDCTR_ATOMIC_STORE = 0x2A,

  LSC_LOAD_QUAD_MSRT = 0x31,
  LSC_STORE_QUAD_MSRT = 0x32,
  LSC_INVALID = 0xFFFFFFFF,
};

/////////////////////////////////////////////////////////////////////
// The vISA spec depends on the enum values assigned.  If you really
// need to change them, fix any consumers encoders dependent on the
// encoding (e.g. CM binary encoder) as well as the vISA spec.
// This block just spot-checks a few of them.
// NOTE: we can remove these constraints once we are rid of the vISA binary format.
static_assert(LSC_LOAD == 0x0,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_FENCE == 0x1F,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_DATA_ELEMS_8 == 5,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_ADDR_SIZE_16b == 1,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_ADDR_SIZE_64b == 3,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_DATA_SIZE_8b == 1,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_DATA_SIZE_16b == 2,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_DATA_SIZE_32b == 3,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_DATA_SIZE_64b == 4,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_DATA_SIZE_16c32bH == 7,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_DATA_ORDER_NONTRANSPOSE == 1,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_DATA_ORDER_TRANSPOSE == 2,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_ADDR_TYPE_FLAT == 1,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_ADDR_TYPE_BTI == 4,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_CACHING_DEFAULT == 0,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_CACHING_READINVALIDATE == 6,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_FENCE_OP_NONE == 0,
              "vISA binary encoding depends on enum ordinal value");
static_assert(LSC_FENCE_OP_FLUSHL3 == 5,
              "vISA binary encoding depends on enum ordinal value");

// FixedFunctionID: these are hardware FFID values
enum FFID {
  FFID_NULL = 0x0,
  FFID_VSR = 0x3,
  FFID_HS = 0x4,
  FFID_DS = 0x5,
  FFID_TS = 0x6,
  FFID_GP = 0x7,
  FFID_GP1 = 0x8,
  FFID_VS = 0x9,
  FFID_GS = 0xC,
  FFID_PS = 0xF,

  FFID_INVALID = 0xFF
};

// Option vISA_lscEnableImmOffsFor (-lscEnableImmOffsFor) will be a bitmask
// of these; vISA will attempt enable/disable immediate offsets based on this
// bitset
typedef enum {
  VISA_LSC_IMMOFF_INVALID,
  VISA_LSC_IMMOFF_ADDR_TYPE_FLAT = 1,
  VISA_LSC_IMMOFF_ADDR_TYPE_BSS = 2,
  VISA_LSC_IMMOFF_ADDR_TYPE_SS = 3,
  VISA_LSC_IMMOFF_ADDR_TYPE_BTI = 4,
  VISA_LSC_IMMOFF_ADDR_TYPE_XXXX = 5,
  VISA_LSC_IMMOFF_PAYLOAD_LOADING = 16,
  VISA_LSC_IMMOFF_SPILL_FILL = 17
} VISALscImmOffOpts;


#endif // _VISA_IGC_COMMON_HEADER_H_
