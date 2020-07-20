/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#ifndef _VISA_IGC_COMMON_HEADER_H_
#define _VISA_IGC_COMMON_HEADER_H_


typedef enum
{
    vISA_3D,
    vISA_MEDIA,
    vISA_PARSER,
    vISA_ASM_WRITER,    /* Builder mode for writing inline visa asm stream */
    vISA_ASM_READER     /* Builder mode for reading inline visa asm stream */
} vISABuilderMode;

typedef enum
{
    PREDEFINED_NULL             = 0,
    PREDEFINED_X                = 1,
    PREDEFINED_Y                = 2,
    PREDEFINED_GROUP_ID_X       = 3,
    PREDEFINED_GROUP_ID_Y       = 4,
    PREDEFINED_GROUP_ID_Z       = 5,
    PREDEFINED_TSC              = 6,
    PREDEFINED_R0               = 7,
    PREDEFINED_ARG              = 8,
    PREDEFINED_RET              = 9,
    PREDEFINED_FE_SP            = 10,
    PREDEFINED_FE_FP            = 11,
    PREDEFINED_HW_TID           = 12,
    PREDEFINED_SR0              = 13,
    PREDEFINED_CR0              = 14,
    PREDEFINED_CE0              = 15,
    PREDEFINED_DBG              = 16,
    PREDEFINED_COLOR            = 17,
    PREDEFINED_VAR_LAST         = PREDEFINED_COLOR
} PreDefined_Vars;

typedef enum
{
    // FIXME: why not expose PREDEF_SURF_0 etc. directly?
    PREDEFINED_SURFACE_SLM = 0,
    PREDEFINED_SURFACE_STACK = 1,
    PREDEFINED_SURFACE_T252 = 4,
    PREDEFINED_SURFACE_T255 = 5,
    PREDEFINED_SURFACE_LAST = PREDEFINED_SURFACE_T255
} PreDefined_Surface;

typedef enum {
    ATOMIC_ADD                     = 0x0,
    ATOMIC_SUB                     = 0x1,
    ATOMIC_INC                     = 0x2,
    ATOMIC_DEC                     = 0x3,
    ATOMIC_MIN                     = 0x4,
    ATOMIC_MAX                     = 0x5,
    ATOMIC_XCHG                    = 0x6,
    ATOMIC_CMPXCHG                 = 0x7,
    ATOMIC_AND                     = 0x8,
    ATOMIC_OR                      = 0x9,
    ATOMIC_XOR                     = 0xa,
    ATOMIC_IMIN                    = 0xb,
    ATOMIC_IMAX                    = 0xc,
    ATOMIC_PREDEC                  = 0xd,
    ATOMIC_FMAX                    = 0x10,
    ATOMIC_FMIN                    = 0x11,
    ATOMIC_FCMPWR                  = 0x12,
    ATOMIC_UNDEF
} VISAAtomicOps;

/*
 * Various enumerations representing the binary encoding definitions for the common ISA
 *
 */
 typedef enum : unsigned char {
     ISA_TYPE_UD    = 0x0,
     ISA_TYPE_D     = 0x1,
     ISA_TYPE_UW    = 0x2,
     ISA_TYPE_W     = 0x3,
     ISA_TYPE_UB    = 0x4,
     ISA_TYPE_B     = 0x5,
     ISA_TYPE_DF    = 0x6,
     ISA_TYPE_F     = 0x7,
     ISA_TYPE_V     = 0x8,
     ISA_TYPE_VF    = 0x9,
     ISA_TYPE_BOOL  = 0xA,
     ISA_TYPE_UQ    = 0xB,
     ISA_TYPE_UV    = 0xC,
     ISA_TYPE_Q     = 0xD,
     ISA_TYPE_HF    = 0xE,
     ISA_TYPE_NUM
 } VISA_Type ;

 typedef struct {
    VISA_Type CISAType;
    const char* typeName;
    int typeSize;
} CISATypeInfo;

extern CISATypeInfo CISATypeTable[ISA_TYPE_NUM];

typedef enum {
    ALIGN_UNDEF = 0x0,
    ALIGN_WORD = 0x1,
    ALIGN_DWORD = 0x2,
    ALIGN_QWORD = 0x3,
    ALIGN_OWORD = 0x4,
    ALIGN_GRF = 0x5,
    ALIGN_2_GRF = 0x6,
    ALIGN_HWORD = 0x7,
    ALIGN_32WORD = 0x8,
    ALIGN_64WORD = 0x9,
    ALIGN_BYTE = 0x0
} VISA_Align;

typedef enum {
    LABEL_BLOCK = 0x0,
    LABEL_SUBROUTINE = 0x1,
    LABEL_FC = 0x2,
    LABEL_FUNCTION = 0x3
} VISA_Label_Kind;

typedef enum {
    ISA_CMP_E = 0,         /* Zero or Equal */
    ISA_CMP_NE,        /* Not Zero or Not Equal */
    ISA_CMP_G,         /* Greater-than */
    ISA_CMP_GE,        /* Greater-than-or-equal */
    ISA_CMP_L,         /* Less-than */
    ISA_CMP_LE,        /* Less-than-or-equal */
    ISA_CMP_UNDEF
} VISA_Cond_Mod;

typedef enum {
    MODIFIER_NONE = 0x0,
    MODIFIER_ABS = 0x1,
    MODIFIER_NEG = 0x2,
    MODIFIER_NEG_ABS = 0x3,
    MODIFIER_SAT = 0x4,
    MODIFIER_NOT = 0x5  //BDW only
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
    vISA_EMASK_M1       = 0,
    vISA_EMASK_M2       = 1,
    vISA_EMASK_M3       = 2,
    vISA_EMASK_M4       = 3,
    vISA_EMASK_M5       = 4,
    vISA_EMASK_M6       = 5,
    vISA_EMASK_M7       = 6,
    vISA_EMASK_M8       = 7,
    vISA_EMASK_M1_NM    = 8,
    vISA_EMASK_M2_NM    = 9,
    vISA_EMASK_M3_NM    = 10,
    vISA_EMASK_M4_NM    = 11,
    vISA_EMASK_M5_NM    = 12,
    vISA_EMASK_M6_NM    = 13,
    vISA_EMASK_M7_NM    = 14,
    vISA_EMASK_M8_NM    = 15,
    vISA_NUM_EMASK      = 16
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
    VME_STREAM_OUT     = 1,
    VME_STREAM_IN      = 2,
    VME_STREAM_IN_OUT  = 3
} COMMON_ISA_VME_STREAM_MODE;

typedef enum {
    VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START = 0,
    VME_SEARCH_SINGLE_REF_SINGLE_REC_DUAL_START   = 1,
    VME_SEARCH_SINGLE_REF_DUAL_REC                = 3,
    VME_SEARCH_DUAL_REF_DUAL_REC                  = 7
} COMMON_ISA_VME_SEARCH_CTRL;

typedef enum
{
    CISA_DM_FMIN = 0,
    CISA_DM_FMAX = 1
} CISA_MIN_MAX_SUB_OPCODE;

typedef enum
{
    CISA_PLANE_Y = 0,
    CISA_PLANE_UV = 1,
    CISA_PLANE_V = 2
} CISA_PLANE_ID;

typedef enum {
    /* 0x0 reserved -- used for illegal/nop op? */
    ISA_RESERVED_0         = 0x0 ,
    ISA_ADD                = 0x1 ,
    ISA_AVG                = 0x2 ,
    ISA_DIV                = 0x3 ,
    ISA_DP2                = 0x4 ,
    ISA_DP3                = 0x5 ,
    ISA_DP4                = 0x6 ,
    ISA_DPH                = 0x7 ,
    ISA_EXP                = 0x8 ,
    ISA_FRC                = 0x9 ,
    ISA_LINE               = 0xA ,
    ISA_LOG                = 0xB ,
    ISA_MAD                = 0xC ,
    ISA_MULH               = 0xD ,
    ISA_LRP                = 0xE ,
    ISA_MOD                = 0xF ,
    ISA_MUL                = 0x10,
    ISA_POW                = 0x11,
    ISA_RNDD               = 0x12,
    ISA_RNDU               = 0x13,
    ISA_RNDE               = 0x14,
    ISA_RNDZ               = 0x15,
    ISA_SAD2               = 0x16,
    ISA_SIN                = 0x17,
    ISA_COS                = 0x18,
    ISA_SQRT               = 0x19,
    ISA_RSQRT              = 0x1A,
    ISA_INV                = 0x1B,
    ISA_RESERVED_1C        = 0x1C,
    ISA_RESERVED_1D        = 0x1D,
    ISA_RESERVED_1E        = 0x1E,
    ISA_LZD                = 0x1F,
    ISA_AND                = 0x20,
    ISA_OR                 = 0x21,
    ISA_XOR                = 0x22,
    ISA_NOT                = 0x23,
    ISA_SHL                = 0x24,
    ISA_SHR                = 0x25,
    ISA_ASR                = 0x26,
    ISA_CBIT               = 0x27,
    ISA_ADDR_ADD           = 0x28,
    ISA_MOV                = 0x29,
    ISA_SEL                = 0x2A,
    ISA_SETP               = 0x2B,
    ISA_CMP                = 0x2C,
    ISA_MOVS               = 0x2D,
    ISA_FBL                = 0x2E,
    ISA_FBH                = 0x2F,
    ISA_SUBROUTINE         = 0x30,
    ISA_LABEL              = 0x31,
    ISA_JMP                = 0x32,
    ISA_CALL               = 0x33,
    ISA_RET                = 0x34,
    ISA_OWORD_LD           = 0x35,
    ISA_OWORD_ST           = 0x36,
    ISA_MEDIA_LD           = 0x37,
    ISA_MEDIA_ST           = 0x38,
    ISA_GATHER             = 0x39,
    ISA_SCATTER            = 0x3A,
    ISA_RESERVED_3B        = 0x3B,
    ISA_OWORD_LD_UNALIGNED = 0x3C,
    ISA_RESERVED_3D        = 0x3D,
    ISA_RESERVED_3E        = 0x3E,
    ISA_RESERVED_3F        = 0x3F,
    ISA_SAMPLE             = 0x40,
    ISA_SAMPLE_UNORM       = 0x41,
    ISA_LOAD               = 0x42,
    ISA_AVS                = 0x43,
    ISA_VA                 = 0x44,
    ISA_FMINMAX            = 0x45,
    ISA_BFE                = 0x46,
    ISA_BFI                = 0x47,
    ISA_BFREV              = 0x48,
    ISA_ADDC               = 0x49,
    ISA_SUBB               = 0x4A,
    ISA_GATHER4_TYPED      = 0x4B,
    ISA_SCATTER4_TYPED     = 0x4C,
    ISA_VA_SKL_PLUS        = 0x4D,
    ISA_SVM                = 0x4E,
    ISA_IFCALL             = 0x4F,
    ISA_FADDR              = 0x50,
    ISA_FILE               = 0x51,
    ISA_LOC                = 0x52,
    ISA_RESERVED_53        = 0x53,
    ISA_VME_IME            = 0x54,
    ISA_VME_SIC            = 0x55,
    ISA_VME_FBR            = 0x56,
    ISA_VME_IDM            = 0x57,
    ISA_RESERVED_58        = 0x58,
    ISA_BARRIER            = 0x59,
    ISA_SAMPLR_CACHE_FLUSH = 0x5A,
    ISA_WAIT               = 0x5B,
    ISA_FENCE              = 0x5C,
    ISA_RAW_SEND           = 0x5D,
    ISA_RESERVED_5E        = 0x5E,
    ISA_YIELD              = 0x5F,
    ISA_RESERVED_60        = 0x60,
    ISA_RESERVED_61        = 0x61,
    ISA_RESERVED_62        = 0x62,
    ISA_RESERVED_63        = 0x63,
    ISA_RESERVED_64        = 0x64,
    ISA_RESERVED_65        = 0x65,
    ISA_RESERVED_66        = 0x66,
    ISA_FCALL              = 0x67,
    ISA_FRET               = 0x68,
    ISA_SWITCHJMP          = 0x69,
    ISA_SAD2ADD            = 0x6A,
    ISA_PLANE              = 0x6B,
    ISA_GOTO               = 0x6C,
    ISA_3D_SAMPLE          = 0x6D,
    ISA_3D_LOAD            = 0x6E,
    ISA_3D_GATHER4         = 0x6F,
    ISA_3D_INFO            = 0x70,
    ISA_3D_RT_WRITE        = 0x71,
    ISA_3D_URB_WRITE       = 0x72,
    ISA_3D_TYPED_ATOMIC    = 0x73,
    ISA_GATHER4_SCALED     = 0x74,
    ISA_SCATTER4_SCALED    = 0x75,
    ISA_RESERVED_76        = 0x76,
    ISA_RESERVED_77        = 0x77,
    ISA_GATHER_SCALED      = 0x78,
    ISA_SCATTER_SCALED     = 0x79,
    ISA_RAW_SENDS          = 0x7A,
    ISA_LIFETIME           = 0x7B,
    ISA_SBARRIER           = 0x7C,
    ISA_DWORD_ATOMIC       = 0x7D,
    ISA_SQRTM              = 0x7E,
    ISA_DIVM               = 0x7F,
    ISA_ROL                = 0x80,
    ISA_ROR                = 0x81,
    ISA_DP4A               = 0x82,
        ISA_NUM_OPCODE,
    ISA_OPCODE_ENUM_SIZE   = 0xFF
} ISA_Opcode;

typedef enum
{
    VISA_3D_SAMPLE          = 0,
    VISA_3D_SAMPLE_B        = 1,
    VISA_3D_SAMPLE_L        = 2,
    VISA_3D_SAMPLE_C        = 3,
    VISA_3D_SAMPLE_D        = 4,
    VISA_3D_SAMPLE_B_C      = 5,
    VISA_3D_SAMPLE_L_C      = 6,
    VISA_3D_LD              = 7,
    VISA_3D_GATHER4         = 8,
    VISA_3D_LOD             = 9,
    VISA_3D_RESINFO         = 10,
    VISA_3D_SAMPLEINFO      = 11,
    VISA_3D_SAMPLE_KILLPIX  = 12,
    VISA_3D_GATHER4_C       = 16,
    VISA_3D_GATHER4_PO      = 17,
    VISA_3D_GATHER4_PO_C    = 18,
    VISA_3D_SAMPLE_D_C      = 20,
    VISA_3D_SAMPLE_LZ       = 24,
    VISA_3D_SAMPLE_C_LZ     = 25,
    VISA_3D_LD_LZ           = 26,
    VISA_3D_LD2DMS_W        = 28,
    VISA_3D_LD_MSC          = 29,
    VISA_3D_TOTAL_NUM_OPS   = 30
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
    CHANNEL_16_BIT_FULL        = 0,
    CHANNEL_16_BIT_DOWNSAMPLED = 1,
    CHANNEL_8_BIT_FULL         = 2,
    CHANNEL_8_BIT_DOWNSAMPLED  = 3,
    CHANNEL_OUTPUT_NUM         = 4
} CHANNEL_OUTPUT_FORMAT ;

typedef enum
{
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
    ALL
} TARGET_PLATFORM;


typedef enum {
    Step_A      = 0,
    Step_B      = 1,
    Step_C      = 2,
    Step_D      = 3,
    Step_E      = 4,
    Step_F      = 5,
    Step_none   = 6  // make this the last one so comparison would work
} Stepping;

// gather/scatter element size
typedef enum {
    GATHER_SCATTER_BYTE = 0x0,
    GATHER_SCATTER_WORD = 0x1,
    GATHER_SCATTER_DWORD = 0x2,
    GATHER_SCATTER_BYTE_UNDEF
} GATHER_SCATTER_ELEMENT_SIZE;

/// Sampler8x8 DevBDW+ Functionality Opcodes
typedef enum VA_fopcode
{
    AVS_FOPCODE                       = 0x00,
    Convolve_FOPCODE                  = 0x01,
    MINMAX_FOPCODE                    = 0x02,
    MINMAXFILTER_FOPCODE              = 0x03,
    ERODE_FOPCODE                     = 0x04,
    Dilate_FOPCODE                    = 0x05,
    BoolCentroid_FOPCODE              = 0x06,
    Centroid_FOPCODE                  = 0x07,
    VA_OP_CODE_1D_CONVOLVE_VERTICAL   = 0x08,
    VA_OP_CODE_1D_CONVOLVE_HORIZONTAL = 0x09,
    VA_OP_CODE_1PIXEL_CONVOLVE        = 0x0A,
    VA_OP_CODE_FLOOD_FILL             = 0x0B,
    VA_OP_CODE_LBP_CREATION           = 0x0C,
    VA_OP_CODE_LBP_CORRELATION        = 0x0D,
    VA_OP_CODE_NONE                   = 0x0E,
    VA_OP_CODE_CORRELATION_SEARCH     = 0x0F,
    ISA_HDC_CONV                      = 0x10,
    ISA_HDC_MMF                       = 0x11,
    ISA_HDC_ERODE                     = 0x12,
    ISA_HDC_DILATE                    = 0x13,
    ISA_HDC_LBPCORRELATION            = 0x14,
    ISA_HDC_LBPCREATION               = 0x15,
    ISA_HDC_1DCONV_H                  = 0x16,
    ISA_HDC_1DCONV_V                  = 0x17,
    ISA_HDC_1PIXELCONV                = 0x18,
    VA_OP_CODE_UNDEFINED              = 0x19
} ISA_VA_Sub_Opcode;

typedef enum _OutputFormatControl_
{   AVS_16_FULL        = 0,
    AVS_16_DOWN_SAMPLE = 1,
    AVS_8_FULL         = 2,
    AVS_8_DOWN_SAMPLE  = 3
} OutputFormatControl;

typedef enum _AVSExecMode_
{   AVS_16x4 = 0,
    AVS_8x4  = 1,
    AVS_16x8 = 2,
    AVS_4x4  = 3
} AVSExecMode;

typedef enum _MMFExecMode_
{   VA_MMF_16x4 = 0,
    VA_MMF_16x1 = 2,
    VA_MMF_1x1  = 3
} MMFExecMode;

typedef enum _MMFEnableMode_
{   VA_MINMAX_ENABLE = 0,
    VA_MAX_ENABLE    = 1,
    VA_MIN_ENABLE    = 2
} MMFEnableMode;

typedef enum _CONVExecMode_
{   VA_CONV_16x4 = 0,
    VA_CONV_16x1 = 2
} CONVExecMode;

typedef enum _EDExecMode_
{   VA_ED_64x4 = 0,
    VA_ED_32x4 = 1,
    VA_ED_64x1 = 2,
    VA_ED_32x1 = 3
} EDExecMode;

typedef enum _EDMode_
{   VA_ERODE  = 4,
    VA_DILATE = 5
} EDMode;

typedef enum _LBPCreationMode_
{
    VA_3x3_AND_5x5  = 0,
    VA_3x3          = 1,
    VA_5x5          = 2
} LBPCreationMode;

typedef enum _Convolve1DDirection_
{
    VA_H_DIRECTION = 0,
    VA_V_DIRECTION = 1
} Convolve1DDirection;

typedef enum _CONV1PixelExecMode_
{   VA_CONV1P_16x4 = 0,
    VA_CONV1P_16x1 = 2,
    VA_CONV1P_1x1  = 3
} CONV1PixelExecMode;

typedef enum _HDCReturnFormat_
{
    VA_HDC_CONVOVLE_Y16 = 0,
    VA_HDC_CONVOLVE_Y8  = 1
} HDCReturnFormat;

typedef enum _CONVHDCRegionSize_
{
    VA_HDC_CONVOLVE_15x15 = 0,
    VA_HDC_CONVOLVE_31x31 = 1
} CONVHDCRegionSize;

typedef enum
{
    SVM_BLOCK_NUM_1 = 0x0,
    SVM_BLOCK_NUM_2 = 0x1,
    SVM_BLOCK_NUM_4 = 0x2,
    SVM_BLOCK_NUM_8 = 0x3
} VISA_SVM_Block_Num;

typedef enum
{
    SVM_BLOCK_TYPE_BYTE  = 0x0,
    SVM_BLOCK_TYPE_DWORD = 0x1,
    SVM_BLOCK_TYPE_QWORD = 0x2
} VISA_SVM_Block_Type;


typedef struct _vISA_RT_CONTROLS
{
    unsigned s0aPresent:1;  //src0 Alpha
    unsigned oMPresent:1;   //oMask
    unsigned zPresent:1;    //depth
    unsigned RTIndexPresent : 1; // Whether need to set RTIndex in header
    unsigned isLastWrite:1; //is last RT Write, sets Last Render Target Select bit
    unsigned isPerSample:1;  //Enables Per Sample Render Target Write
    unsigned isStencil:1;
    unsigned isCoarseMode:1; //controls coasrse mode bit inmsg descriptor
    unsigned isSampleIndex : 1; //controls whether sampleIndex is used.
    unsigned isHeaderMaskfromCe0 : 1;
    unsigned isNullRT : 1;   // null render target
} vISA_RT_CONTROLS;

typedef enum
{
    LIFETIME_START = 0,
    LIFETIME_END = 1
} VISAVarLifetime;



// FixedFunctionID: these are hardware FFID values
enum FFID
{
    FFID_NULL = 0x0,
    FFID_VSR  = 0x3,
    FFID_HS   = 0x4,
    FFID_DS   = 0x5,
    FFID_TS   = 0x6,
    FFID_GP   = 0x7,
    FFID_GP1  = 0x8,
    FFID_VS   = 0x9,
    FFID_GS   = 0xC,
    FFID_PS   = 0xF,

    FFID_INVALID = 0xFF
};

#endif
