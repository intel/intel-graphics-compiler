/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include <stdint.h>

///
/// ISA Description
///

#define TYPE_INTEGER ISA_TYPE_UW|ISA_TYPE_W|ISA_TYPE_UB|ISA_TYPE_B|ISA_TYPE_D|ISA_TYPE_UD|ISA_TYPE_Q|ISA_TYPE_UQ
#define TYPE_FLOAT   ISA_TYPE_DF|ISA_TYPE_F
#define TYPE_FLOAT_ALL   ISA_TYPE_DF|ISA_TYPE_F|ISA_TYPE_HF
#define TYPE_ANY TYPE_INTEGER | TYPE_FLOAT

#define SIZEOF_CISA_OPCODE sizeof(unsigned char)
#define OPND_DST_GEN       0x100
#define OPND_SRC_GEN       0x200
#define OPND_DST_INDIR     0x400
#define OPND_SRC_INDIR     0x800
#define OPND_DST_PRED      0x1000
#define OPND_SRC_PRED      0x2000
#define OPND_DST_ADDR      0x4000
#define OPND_SRC_ADDR      0x8000
#define OPND_ADDRESS_OF    0x10000
#define OPND_SURFACE       0x20000
#define OPND_SAMPLE        0x40000
#define OPND_IMM           0x100000
#define OPND_PRED          0x200000
#define OPND_OTHER         0x400000
#define OPND_RAW_SRC       0x800000
#define OPND_RAW_DST       0x1000000

#define OPND_VECTOR_SRC_G_IMM_AO     OPND_SRC_GEN | OPND_IMM | OPND_ADDRESS_OF
#define OPND_VECTOR_SRC_G_I_IMM_AO   OPND_SRC_GEN | OPND_IMM |OPND_SRC_INDIR | OPND_ADDRESS_OF
#define OPND_VECTOR_SRC_G_I_IMM      OPND_SRC_GEN | OPND_IMM |OPND_SRC_INDIR
#define OPND_VECTOR_SRC_G_I_IMM_A_AO OPND_SRC_GEN | OPND_IMM |OPND_SRC_INDIR | OPND_SRC_ADDR | OPND_ADDRESS_OF
#define OPND_VECTOR_SRC_G_I_IMM_P_AO OPND_SRC_GEN | OPND_IMM |OPND_SRC_INDIR | OPND_SRC_PRED | OPND_ADDRESS_OF
#define OPND_VECTOR_SRC_G_A_AO       OPND_SRC_GEN | OPND_SRC_ADDR | OPND_ADDRESS_OF
#define OPND_VECTOR_SRC_G_I          OPND_SRC_GEN | OPND_SRC_INDIR

#define OPND_VECTOR_DST_G_I          OPND_DST_GEN | OPND_DST_INDIR
#define OPND_VECTOR_DST_G_I_A        OPND_DST_GEN | OPND_DST_INDIR | OPND_DST_ADDR
#define OPND_VECTOR_DST_G_I_P        OPND_DST_GEN | OPND_DST_PRED | OPND_DST_INDIR

#define OPND_VECTOR_SRC              OPND_SRC_GEN | OPND_IMM |OPND_SRC_INDIR | OPND_SRC_ADDR | OPND_ADDRESS_OF | OPND_SRC_PRED
#define OPND_VECTOR_DST              OPND_DST_GEN | OPND_DST_INDIR | OPND_DST_ADDR | OPND_DST_PRED

#define OPND_SPECIAL                 OPND_SAMPLE | OPND_SURFACE

#define SAME_DATA_TYPE    0x1
#define SAME_SPECIAL_KIND 0x2

#define OPND_BLOCK_WIDTH  OPND_IMM
#define OPND_BLOCK_HEIGHT OPND_IMM
#define OPND_PLANE        OPND_IMM

#define OPND_SIMB_INDEX   OPND_IMM
#define OPND_NUM_OPNDS    OPND_IMM
#define OPND_KIND         OPND_IMM

typedef enum {
    SIZE_1 = 1,
    SIZE_2 = 2,
    SIZE_4 = 4,
    SIZE_8 = 8
} SpecificSize;

typedef enum {
    HORIZON_STRIDE_1 = 1,
    HORIZON_VERTICAL_STRIDE_0,
    HORIZON_STRIDE_2,
    ELEM_NUM_2,
    ELEM_NUM_4,
    ELEM_NUM_8_16,
    ELEM_NUM_96,
    ELEM_NUM_128,
    ELEM_NUM_224,
    ELEM_NUM_GE_2,
    ELEM_NUM_GE_16,
    ELEM_NUM_GE_32,
    ELEM_NUM_GE_128,
    ELEM_NUM_GE_160,
    ELEM_NUM_MC32,
    ELEM_NUM_MC16,
    SIZE_54,
    SIZE_128,
    SIZE_192,
    SIZE_224,
    SIZE_228,
    SIZE_352,
    SIZE_SIZE,
    OWORD_SIZE,
    GE_4,
    VALUE_0_3,
    VALUE_1_32,
    VALUE_1_64,
    SINGLE_DATA_TYPE,
    PREDICATE_NONEPRED_OPND,
    SCALAR_REGION,
    LABEL_BLOCK_C,
    LABEL_FUNC_C,
    SIZE_GE_WIDTH_M_HIEGH,
    GE_READSIZE,
    GE_WRITESIZE,
    SIZE_STREAM_MODE_DEPENDENT_1,
    SIZE_STREAM_MODE_DEPENDENT_2,
    SIZE_STREAM_MODE_DEPENDENT_3,
    SIZE_STREAM_MODE_DEPENDENT_4,
    LENGHT_LESS_256,
    GRF_ALIGNED = 0x100,
    SAT_C = 0x200,
    SAT_FLOAT_ONLY = 0x400

    //GATHER: UPPER_BITS_IGNORE,
    // LINENUM: LARGE_THAN_0,
    //SIZE_BLOCK_HEIGH_WIDTH,
    //OWORD_LD_UNALIGNED: SIZE_SIZE_OWORD,
    //Instruction specific features
    //RIGHT_ALIGNED,
    //MOVS:  SINGLE_SPEC_OPND_TYPE,
    //FILE NAME: LENGHT_LESS_256,
    //ALL:  WITHIN_SIMD_WIDTH
} OpndContraint;

//Common_ISA_Opnd_Desc_Type
enum {
     OPND_EXECSIZE = 1,
     OPND_STRING,
     OPND_LABEL,
     OPND_ATOMIC_SUBOP,
     OPND_EMASK_CTRL,
     OPND_COND_MODE,
     OPND_CHAN_PATT,
     OPND_OWORD_SIZE,
     OPND_IS_MODIFIED,
     OPND_ELEM_NUM,
     OPND_ELEM_SIZE,
     OPND_SIMD_MODE,
     OPND_CHANNEL_SIMD_MODE,
     OPND_CMP_SUBOP,
     OPND_VME_SUBOP,
     OPND_STREAM_MODE,
     OPND_SEARCH_CRTL,
     OPND_MATRIX_MODE,
     OPND_SUBMATRIX_SHAPE,
     OPND_SUBPRE_SHAPE,
     OPND_SPECIAL_KIND,
     OPND_MEDIA_LD_MODIFIER,
     OPND_MEDIA_ST_MODIFIER,
     OPND_RAW,
     OPND_SUBOPCODE,
     OP_EXT
};

typedef enum
{
    ISA_Inst_Mov       = 0x0,
    ISA_Inst_Arith     = 0x1,
    ISA_Inst_Logic     = 0x2,
    ISA_Inst_Compare   = 0x3, //CMP
    ISA_Inst_Address   = 0x4, //ADDROF, ADDR_ADD
    ISA_Inst_Flow      = 0x5,
    ISA_Inst_Data_Port = 0x6,
    ISA_Inst_Sampler   = 0x7,
    ISA_Inst_Misc      = 0x8, // VME, etc.
    ISA_Inst_SIMD_Flow = 0x9,
    ISA_Inst_Sync      = 0xA,
    ISA_Inst_SVM       = 0xB,
    ISA_Inst_LSC       = 0xC,
    ISA_Inst_Reserved
} ISA_Inst_Type;

struct ISA_Inst_Info
{
    ISA_Opcode    op;
    ISA_Inst_Type type;
    const char*   str;
    uint8_t       n_srcs;  //for send messages, we count the surface as well as all the offsets to be sources
    uint8_t       n_dsts;
};

#define MAX_OPNDS_PER_INST 24

typedef struct OpndDesc
{
    unsigned opnd_type; //Common_ISA_Opnd_Desc_Type OR #defines like OPND_VECTOR_SRC_G_IMM_AO
    unsigned data_type; //VISA_Type, overloaded to supported data types if it's a vector
    unsigned opnd_constraint;
} OpndDesc;


typedef uint8_t ISA_SubOpcode;

struct ISA_SubInst_Desc
{
    ISA_SubOpcode  subOpcode;
    ISA_Inst_Type  type;
    const char*    name;
    uint16_t       opnd_num;
    OpndDesc       opnd_desc[MAX_OPNDS_PER_INST];
};

struct VISA_INST_Desc
{
    TARGET_PLATFORM  platf;
    ISA_SubOpcode    opcode;
    ISA_Inst_Type    type;
    const char*      name;
    uint16_t         opnd_num;
    char             attr;
    OpndDesc         opnd_desc[MAX_OPNDS_PER_INST];

    const ISA_SubInst_Desc& getSubInstDesc(uint8_t subOpcode) const;
    const ISA_SubInst_Desc& getSubInstDescByName(const char *symbol) const;
};

enum SVMSubOpcode
{
    SVM_BLOCK_LD = 0x1,
    SVM_BLOCK_ST = 0x2,
    SVM_GATHER   = 0x3,
    SVM_SCATTER  = 0x4,
    SVM_ATOMIC   = 0x5,
    SVM_GATHER4SCALED,
    SVM_SCATTER4SCALED,
    SVM_LASTOP
};

struct LscOpInfo {
    enum OpKind {
      LOAD,
      STORE,
      ATOMIC,
      OTHER
    }           kind;
    LSC_OP      op;
    uint32_t    encoding; // Desc[5:0]
    const char *mnemonic;
    int         extraOperands; // e.g. for atomics (0 for inc, 1 for add, 2 for cas)

    // general op category queries
    // these groups are equivalence classes
    // (i.e. only one will be true for any given op)
    bool isLoad() const {return kind == OpKind::LOAD;}
    bool isStore() const {return kind == OpKind::STORE;}
    bool isAtomic() const {return kind == OpKind::ATOMIC;}
    bool isOther() const {return kind == OpKind::OTHER;}

    // other queries
    bool hasChMask() const {
        return op == LSC_LOAD_QUAD || op == LSC_STORE_QUAD;
    }
    bool isStrided() const {
        return op == LSC_LOAD_STRIDED || op == LSC_STORE_STRIDED;
    }
    bool isBlock2D() const {
        return op == LSC_LOAD_BLOCK2D || op == LSC_STORE_BLOCK2D;
    }
};

LscOpInfo LscOpInfoGet(LSC_OP op); // hard failure
bool LscOpInfoFind(LSC_OP op, LscOpInfo &opInfo); // soft failure

extern struct ISA_Inst_Info ISA_Inst_Table[ISA_OPCODE_ENUM_SIZE];

extern VISA_INST_Desc CISA_INST_table[ISA_NUM_OPCODE];
