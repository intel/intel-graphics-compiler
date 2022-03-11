/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef COMMON_ISA_OPCODE_INCLUDED
#define COMMON_ISA_OPCODE_INCLUDED
namespace vISA
{
class G4_Operand;
class G4_Declare;
}

#include "visa_igc_common_header.h"
#include "IsaDescription.h"
#include "common.h"
#include <string>
#include <cctype>

/*
 * Constant literals for the common ISA
 *
 */
#define COMMON_ISA_MAGIC_NUM 0x41534943

#define COMMON_ISA_MAJOR_VER 4
#define COMMON_ISA_MINOR_VER 0

#define COMMON_ISA_MAX_ADDRESS_SIZE     16
#define COMMON_ISA_MAX_SURFACE_SIZE     128
#define COMMON_ISA_MAX_SAMPLER_SIZE     128
#define COMMON_ISA_MAX_NUM_SURFACES     256
#define COMMON_ISA_MAX_NUM_SAMPLERS     32
#define COMMON_ISA_MAX_NUM_INPUTS       256

// V0-V31 are reserved
#define COMMON_ISA_NUM_PREDEFINED_VAR_VER_3   32
// reserve p0 for the case of no predication
#define COMMON_ISA_NUM_PREDEFINED_PRED  1

// Reserve T0-T5 as special surfaces
#define COMMON_ISA_NUM_PREDEFINED_SURF_VER_3_1  6

// bfi can have 7 operands
#define COMMON_ISA_MAX_NUM_OPND_ARITH_LOGIC 7
#define COMMON_ISA_MAX_NUM_DST 2
#define COMMON_ISA_MAX_NUM_SRC 4

#define COMMON_ISA_MAX_MEDIA_BLOCK_WIDTH_BDW_PLUS 64
#define COMMON_ISA_MAX_MEDIA_BLOCK_WIDTH 32
#define COMMON_ISA_MAX_MEDIA_BLOCK_HEIGHT 64

#define COMMON_ISA_MAX_FILENAME_LENGTH   1023

#define COMMON_ISA_MAX_KERNEL_NAME_LEN  255

#define SEND_GT_MSG_TYPE_BIT                        14
#define SEND_GT_MSG_LENGTH_BIT_OFFSET               25
#define SEND_GT_RSP_LENGTH_BIT_OFFSET               20
#define SEND_GT_MAX_RESPONSE_LENGTH                 16
#define SEND_GT_MAX_MESSAGE_LENGTH                  15
#define SEND_GT_MSG_HEADER_PRESENT_BIT_OFFSET       19

typedef enum {
    GENERAL_VAR,
    ADDRESS_VAR,
    PREDICATE_VAR,
    SAMPLER_VAR,
    SURFACE_VAR,
    LABEL_VAR,
    NUM_VAR_CLASS
} Common_ISA_Var_Class;

typedef enum {
    INPUT_GENERAL   = 0x0,
    INPUT_SAMPLER   = 0x1,
    INPUT_SURFACE   = 0x2,
    INPUT_UNKNOWN
} Common_ISA_Input_Class;


typedef enum {
    INPUT_EXPLICIT          = 0x0,
    LOCAL_SIZE              = 0x1,
    GROUP_COUNT             = 0x2,
    LOCAL_ID                = 0x3,
    PSEUDO_INPUT            = 0x10,
    IMPLICIT_INPUT_COUNT    = 0x5
} Common_ISA_Implicit_Input_Kind;

extern const char * implictKindStrings[IMPLICIT_INPUT_COUNT];

typedef enum {
    OPERAND_GENERAL = 0x0,
    OPERAND_ADDRESS = 0x1,
    OPERAND_PREDICATE = 0x2,
    OPERAND_INDIRECT = 0x3,
    OPERAND_ADDRESSOF = 0x4,
    OPERAND_IMMEDIATE = 0x5,
    OPERAND_STATE = 0x6,
    NUM_OPERAND_CLASS
} Common_ISA_Operand_Class;

typedef enum {
    REGION_NULL = 0x0,
    REGION_0 = 0x1,
    REGION_1 = 0x2,
    REGION_2 = 0x3,
    REGION_4 = 0x4,
    REGION_8 = 0x5,
    REGION_16 = 0x6,
    REGION_32 = 0x7,
    NUM_REGION = 0x8
} Common_ISA_Region_Val;

extern const char* Rel_op_str[ISA_CMP_UNDEF + 1];

extern const char* media_ld_mod_str[MEDIA_LD_Mod_NUM];

// media store inst modifiers
typedef enum {
    MEDIA_ST_nomod = 0x0,
    MEDIA_ST_reserved = 0x1,
    MEDIA_ST_top = 0x2,
    MEDIA_ST_bottom = 0x3,
    MEDIA_ST_Mod_NUM
} MEDIA_ST_mod;

extern const char* media_st_mod_str[MEDIA_ST_Mod_NUM];


extern const char* channel_mask_str[CHANNEL_MASK_NUM];

extern const char* channel_mask_slm_str[CHANNEL_MASK_NUM];

extern const char* sampler_channel_output_str[4];

extern const char* vme_op_mode_str[VME_OP_MODE_NUM];

typedef enum {
    S_OPND_ERROR = 0x0,
    S_OPND_SAMPLER = 0x1,
    S_OPND_SURFACE = 0x2,
    S_OPND_NUM = 0x3
} Common_ISA_State_Opnd;

extern const char* emask_str[];

typedef enum {
    NOT_A_STATE_OPND   = -1,
    STATE_OPND_SURFACE =  0,
    STATE_OPND_SAMPLER,
    STATE_OPND_NUM
} Common_ISA_State_Opnd_Class;

/*
 *  Pseudo-strcutures describing the format of the symbol tables and kernel metadata in the common ISA
 */

struct attribute_info_t {
    uint32_t nameIndex;
    unsigned char size;
    bool isInt;
    union {
        int intVal;
        const char* stringVal;
    } value;

    int getSizeInBinary() const;
};

struct var_info_t {
    uint32_t name_index;
    unsigned char bit_properties;
    unsigned short num_elements;
    uint32_t alias_index;
    unsigned short alias_offset;
    unsigned char alias_scope_specifier;
    unsigned char attribute_capacity;
    unsigned char attribute_count;
    attribute_info_t* attributes;
    vISA::G4_Declare* dcl;  // ToDo: remove this, vISA variables should not have access to internal Gen declares

    VISA_Type getType() const
    {
        return (VISA_Type) (bit_properties & 0xF);
    }

    VISA_Align getAlignment() const
    {
        return (VISA_Align) ((bit_properties >> 4 ) & 0xF);
    }

    VISA_Align getTypeAlignment(unsigned grfSize) const
    {
        VISA_Align typeAlign = ALIGN_WORD;
        if (getSize() >= grfSize)
        {
            typeAlign = grfSize == 64 ? ALIGN_32WORD : ALIGN_HWORD;
        }
        else
        {
            switch (CISATypeTable[getType()].typeSize)
            {
                case 4:
                    typeAlign = ALIGN_DWORD;
                    break;
                case 8:
                    typeAlign = ALIGN_QWORD;
                    break;
                default:
                    // nothing for other types
                    break;
            }
        }
        return typeAlign;
    }

    unsigned int getSize() const
    {
        return num_elements * CISATypeTable[getType()].typeSize;
    }

    int getSizeInBinary() const;
};

struct addr_info_t {
    uint32_t name_index;
    unsigned short num_elements;
    unsigned char attribute_capacity;
    unsigned char attribute_count;
    attribute_info_t* attributes;
    vISA::G4_Declare* dcl;

    int getSizeInBinary() const;
};

struct pred_info_t {
    uint32_t name_index;
    unsigned short num_elements;
    unsigned char attribute_capacity;
    unsigned char attribute_count;
    attribute_info_t* attributes;
    vISA::G4_Declare* dcl;

    int getSizeInBinary() const;
};

struct label_info_t {
    uint32_t name_index;
    unsigned char kind;
    unsigned char attribute_capacity;
    unsigned char attribute_count;
    attribute_info_t* attributes;

    int getSizeInBinary() const;
};

struct state_info_t {
    uint32_t name_index;
    unsigned short num_elements;
    unsigned char attribute_capacity;
    unsigned char attribute_count;
    attribute_info_t* attributes;
    vISA::G4_Declare* dcl;

    int getSizeInBinary() const;
};

//work around for g++
namespace patch
{
    template < typename T > std::string to_string(const T& n)
    {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }
}

struct input_info_t {
    // bits 0-2, category kind
    // bits 3-7, implicit argument kind
    //   0x00 explicit (default)
    //   0x01 local size (3 x ud)
    //   0x02 group count (3 x ud)
    //   0x03 local id (3 x ud)
    //   0x10 pseudo_input
    //   others, reserved.
    uint8_t kind;
    uint32_t index;
    short offset;
    unsigned short size;
    vISA::G4_Declare* dcl;

    inline Common_ISA_Input_Class getInputClass() const
    {
        return (Common_ISA_Input_Class)(kind & 0x7);
    }

    /// Get/set the implicit input kind.
    void setImplicitKind(uint8_t k)
    {
        kind = (kind & 0x7) | (k << 3);
    }
    uint8_t getImplicitKind() const
    {
        return kind >> 3;
    }
    static bool isPseudoInput(uint8_t kind)
    {
        return kind == PSEUDO_INPUT;
    }
    bool isPseudoInput() const
    {
        return isPseudoInput(getImplicitKind());
    }
    static std::string getImplicitKindString(uint16_t kind)
    {
        std::string kindString = ".implicit_";
        if (kind == PSEUDO_INPUT)
        {
            kindString.append(implictKindStrings[4]);
        }
        else if (kind >= IMPLICIT_INPUT_COUNT)
        {
            kindString.append("UNDEFINED_");
            kindString.append(patch::to_string(kind));
        }
        else
        {
            kindString.append(implictKindStrings[kind]);
        }
        return kindString;
    }
    std::string getImplicitKindString() const
    {
        uint32_t kind = getImplicitKind();
        return getImplicitKindString(kind);
    }

    int getSizeInBinary() const;
};

typedef struct {
    char kind;
    unsigned short id;
    short offset;
    unsigned short size;
} parameter_info_t;

typedef struct {
    unsigned short symbolic_index;
    unsigned short resolved_index;
} reloc_sym;

typedef struct {
    unsigned short num_syms;
    reloc_sym* reloc_syms;
} reloc_symtab;

typedef struct {
    unsigned char platform;
    unsigned int binary_offset;
    unsigned int binary_size;
} gen_binary_info;

struct kernel_info_t {
    unsigned short name_len;
    char* name;
    unsigned int offset;
    unsigned int size;
    unsigned int input_offset;
    unsigned int binary_offset;
    unsigned int binary_size;
    reloc_symtab variable_reloc_symtab; // ded, but leave here to avoid breaking old vISA binary
    reloc_symtab function_reloc_symtab; // ded, but leave here to avoid breaking old vISA binary
    unsigned char num_gen_binaries;
    gen_binary_info* gen_binaries;
    // Auxillary data
    //   for cisa binary emmission
    char * cisa_binary_buffer;
    char * genx_binary_buffer;

    uint32_t getSizeInBinary() const;
};

struct function_info_t {
    unsigned char linkage;
    unsigned short name_len;
    char* name;
    unsigned int offset;
    unsigned int size;
    reloc_symtab variable_reloc_symtab; // ded, but leave here to avoid breaking old vISA binary
    reloc_symtab function_reloc_symtab; // ded, but leave here to avoid breaking old vISA binary
    // Auxillary data
    //   for cisa binary emmission
    char* cisa_binary_buffer;
    char* genx_binary_buffer;

    uint32_t getSizeInBinary() const;
};

/*
 *  Format of the common ISA kernel binary.
 *  We do not directly output the kernel struct because the kernel binary is a byte stream
 *  with no padding and alignment for the fields, and because the tables have dynamic length
 *
 */
struct common_isa_header {
    unsigned int          magic_number;
    unsigned char         major_version;
    unsigned char         minor_version;
    unsigned short        num_kernels;
    kernel_info_t*        kernels;
    unsigned short        num_filescope_variables;
    unsigned short        num_functions;
    function_info_t*      functions;

    uint32_t getSizeInBinary() const;
};

typedef struct {
    uint32_t        string_count;
    const char**          strings;
    uint32_t        name_index;
    uint32_t        variable_count;
    var_info_t*           variables;
    unsigned short        address_count;
    addr_info_t*          addresses;
    unsigned short        predicate_count;
    pred_info_t*          predicates;
    unsigned short        label_count;
    label_info_t*         labels;
    unsigned char         sampler_count;
    state_info_t*         samplers;
    unsigned char         surface_count;
    state_info_t*         surfaces;
    unsigned char         vme_count; // deprecated and MBZ
    uint32_t              input_count;
    input_info_t*         inputs;
    unsigned char         return_type;
    unsigned int          size;
    unsigned int          entry;
    unsigned char         input_size;
    unsigned char         return_value_size;
    unsigned short        attribute_count;
    attribute_info_t*     attributes;
    bool*                 surface_attrs;
} kernel_format_t;
typedef kernel_format_t function_format_t;

typedef struct
{
    GenPrecision   Prec;
    unsigned int   BitSize;
    const char*    Name;
} GenPrecision_Info_t;
extern GenPrecision_Info_t GenPrecisionTable[(unsigned int)GenPrecision::TOTAL_NUM];

class print_format_provider_t {
public:
    virtual uint32_t getNameIndex() const = 0;

    virtual const char* getString(uint32_t str_id) const = 0;
    virtual uint32_t getStringCount() const = 0;

    virtual const label_info_t* getLabel(uint16_t label_id) const = 0;
    virtual unsigned short getLabelCount() const = 0;

    virtual const var_info_t* getPredefVar(unsigned var_id) const = 0;
    virtual const var_info_t* getVar(unsigned var_id) const = 0;
    virtual uint32_t getVarCount() const = 0;

    virtual const attribute_info_t* getAttr(unsigned id) const = 0;
    virtual unsigned getAttrCount() const = 0;

    virtual const addr_info_t* getAddr(unsigned id) const = 0;
    virtual unsigned short getAddrCount() const = 0;

    virtual const pred_info_t* getPred(unsigned id) const = 0;
    virtual unsigned short getPredCount() const = 0;

    virtual const state_info_t* getPredefSurface(unsigned id) const = 0;
    virtual const state_info_t* getSurface(unsigned id) const = 0;
    virtual unsigned char getSurfaceCount() const = 0;

    virtual const state_info_t* getSampler(unsigned id) const = 0;
    virtual unsigned char getSamplerCount() const = 0;

    virtual const input_info_t* getInput(unsigned id) const = 0;
    virtual uint32_t getInputCount() const = 0;

};

struct print_decl_index_t {
    unsigned var_index = 0;
    unsigned addr_index = 0;
    unsigned pred_index = 0;
    unsigned sampler_index = 0;
    unsigned surface_index = 0;
    unsigned input_index = 0;
};

struct vector_opnd {
    unsigned char tag;
    union {
        struct {
            uint32_t index;
            unsigned char row_offset;
            unsigned char col_offset;
            unsigned short region;
        } gen_opnd;

        struct {
            unsigned short index;
            unsigned char offset;
            unsigned char width;        // it uses the same def for EXEC_SIZE
        } addr_opnd;

        struct {
            unsigned short index;
        } pred_opnd;

        struct {
            unsigned short index;
            unsigned char addr_offset;
            short indirect_offset;
            unsigned char bit_property;
            unsigned short region;
        } indirect_opnd;

        struct {
            unsigned short index;
            short addr_offset;
        } addressof_opnd;

        struct {
            unsigned char type;
            union {
                unsigned int ival;
                unsigned long long lval;
                double dval;
                float fval;
            } _val;
        } const_opnd;

        struct {
            unsigned char opnd_class;
            unsigned short index;
            unsigned char offset;
        } state_opnd;

    } opnd_val;

    inline Common_ISA_Operand_Class getOperandClass() const
    {
        return (Common_ISA_Operand_Class)(tag & 0x7);
    }

    Common_ISA_State_Opnd_Class getStateOpClass() const
    {
        MUST_BE_TRUE(getOperandClass() == OPERAND_STATE, "state operand expected");
        return (Common_ISA_State_Opnd_Class)opnd_val.state_opnd.opnd_class;
    }

    bool isImmediate() const
    {
        return getOperandClass() == OPERAND_IMMEDIATE;
    }

    VISA_Type getImmediateType() const
    {
        MUST_BE_TRUE(isImmediate(), "immediate constant expected");
        VISA_Type type = (VISA_Type)(opnd_val.const_opnd.type & 0xF);
        MUST_BE_TRUE(type < ISA_TYPE_NUM && type != ISA_TYPE_BOOL,
                     "invalid immediate constant type");
        return type;
    }

    inline VISA_Modifier getOperandModifier() const
    {
        return (VISA_Modifier)((tag >> 3 ) & 0x7);
    }

    inline uint32_t getOperandIndex() const
    {
       switch (getOperandClass())
       {
           case OPERAND_STATE:     return opnd_val.state_opnd     .index;
           case OPERAND_GENERAL:   return opnd_val.gen_opnd       .index;
           case OPERAND_ADDRESS:   return opnd_val.addr_opnd      .index;
           case OPERAND_INDIRECT:  return opnd_val.indirect_opnd  .index;
           case OPERAND_PREDICATE: return opnd_val.pred_opnd      .index;
           case OPERAND_ADDRESSOF: return opnd_val.addressof_opnd .index;
           default:                return 0; ///OPERAND_IMMEDIATE
       }
    }

    int getSizeInBinary() const;
};

typedef struct _raw_opnd{
    uint32_t index;
    unsigned short offset;

   std::string toString() const;
} raw_opnd;

typedef enum
{
    CISA_OPND_VECTOR = 0,
    CISA_OPND_RAW    = 1,
    CISA_OPND_OTHER  = 2
} CISA_opnd_type;

typedef struct _CISA_GEN_VAR
{
    Common_ISA_Var_Class type;
    unsigned int index; //index into respective symbol tables
    union
    {
        var_info_t genVar;
        addr_info_t addrVar;
        pred_info_t predVar;
        state_info_t stateVar;
        label_info_t labelVar;
    };
} CISA_GEN_VAR;

typedef struct _VISA_GenVar     : CISA_GEN_VAR { } VISA_GenVar;
typedef struct _VISA_AddrVar    : CISA_GEN_VAR { } VISA_AddrVar;
typedef struct _VISA_PredVar    : CISA_GEN_VAR { } VISA_PredVar;
typedef struct _VISA_SamplerVar : CISA_GEN_VAR { } VISA_SamplerVar;
typedef struct _VISA_SurfaceVar : CISA_GEN_VAR { } VISA_SurfaceVar;
typedef struct _VISA_LabelVar   : CISA_GEN_VAR { } VISA_LabelVar;

// unfortunately vISA binary restricts the max number of predicates to 4K so that we could pack
// pred id + control into 2 bytes. It seemed like a good idea at the time but our input programs now
// regularly exceed it. Since we don't want to touch legacy binary format the alternative is
// to make it work at least for vISA assembly.
struct PredicateOpnd
{

private:
    uint32_t predId;
    uint16_t predInBinary; // bit[0:11] - LSB 12bit for pred id, bit[13:14] - pred control, bit[15] - pred inverse

public:
    PredicateOpnd() : predId(0), predInBinary(0) {}
    PredicateOpnd(uint32_t id, uint16_t binaryId) : predId(id), predInBinary(binaryId) {}
    PredicateOpnd(uint32_t id, VISA_PREDICATE_STATE state, VISA_PREDICATE_CONTROL cntrl)
    {
        predId = id;
        predInBinary = (id & 0xFFF) | (cntrl << 13) | (state << 15);
    }

    uint32_t getId() const { return predId; }
    bool isNullPred() const { return predId == 0; }
    bool isInverse() const { return predInBinary & 0x8000; }
    VISA_PREDICATE_CONTROL getControl() const
    {
        return (VISA_PREDICATE_CONTROL)((predInBinary & 0x6000) >> 13);
    }

    uint16_t getPredInBinary() const { return predInBinary; }
    static constexpr int getPredInBinarySize() { return sizeof(predInBinary); }

    static PredicateOpnd getNullPred()
    {
        return PredicateOpnd();
    }
};

typedef struct _CISA_opnd
{
    CISA_opnd_type opnd_type;
    unsigned char  tag;   // from opnd description tag
    unsigned short size;  // size of the operand
    uint32_t index; // this should be the same value as index in v_opnd

    union
    {
        vector_opnd  v_opnd;
        raw_opnd     r_opnd;
        unsigned int other_opnd;
    } _opnd;
    vISA::G4_Operand *g4opnd;
    VISA_GenVar *decl;

    PredicateOpnd convertToPred() const
    {
        assert(_opnd.v_opnd.getOperandClass() == OPERAND_PREDICATE);
        return PredicateOpnd(index, _opnd.v_opnd.opnd_val.pred_opnd.index);
    }
} VISA_opnd;

typedef struct _CISA_INST
{
    unsigned char  opcode;
    unsigned char  execsize;
    unsigned char  modifier; /// Mainly used for media ld/store.
    ISA_Inst_Type  isa_type;
    PredicateOpnd  pred;
    VISA_opnd**    opnd_array;
    unsigned       opnd_count;
    unsigned       id;

    VISA_Exec_Size getExecSize() const { return (VISA_Exec_Size) (execsize & 0xF); }
    VISA_EMask_Ctrl getExecMask() const { return (VISA_EMask_Ctrl) (execsize >> 4); }

} CISA_INST;

#define READ_FIELD_FROM_BUF( dst, type ) \
    dst = *((type *) &buf[byte_pos]); \
    byte_pos += sizeof(type);

#define STRING_LEN  1024

struct Common_ISA_Attribute{
    char* name;
    char* value;
};

typedef struct _string_pool_entry {
    Common_ISA_Var_Class type;
    VISA_Type data_type;
    char *value;
    struct _string_pool_entry *next;
} string_pool_entry;

extern const char* CISAAtomicOpNames[];

typedef enum {
    // integer operations
    GEN_ATOMIC_CMPWR_2W                = 0x0,  // AOP_CMPWR_2W
    GEN_ATOMIC_AND                     = 0x1,  // AOP_AND
    GEN_ATOMIC_OR                      = 0x2,  // AOP_OR
    GEN_ATOMIC_XOR                     = 0x3,  // AOP_XOR
    GEN_ATOMIC_MOV                     = 0x4,  // AOP_MOV
    GEN_ATOMIC_INC                     = 0x5,  // AOP_INC
    GEN_ATOMIC_DEC                     = 0x6,  // AOP_DEC
    GEN_ATOMIC_ADD                     = 0x7,  // AOP_ADD
    GEN_ATOMIC_SUB                     = 0x8,  // AOP_SUB
    GEN_ATOMIC_REVSUB                  = 0x9,  // AOP_REVSUB
    GEN_ATOMIC_IMAX                    = 0xa,  // AOP_IMAX
    GEN_ATOMIC_IMIN                    = 0xb,  // AOP_IMIN
    GEN_ATOMIC_UMAX                    = 0xc,  // AOP_UMAX
    GEN_ATOMIC_UMIN                    = 0xd,  // AOP_UMIN
    GEN_ATOMIC_CMPWR                   = 0xe,  // AOP_CMPWR
    GEN_ATOMIC_PREDEC                  = 0xf,  // AOP_PREDEC
    // float operations
    GEN_ATOMIC_FMAX                    = 0x1,  // FOP_FMAX
    GEN_ATOMIC_FMIN                    = 0x2,  // FOP_FMIN
    GEN_ATOMIC_FCMPWR                  = 0x3,  // FOP_FCMPWR
    GEN_ATOMIC_FADD                    = 0x4,  // FOP_FADD
    GEN_ATOMIC_FSUB                    = 0x5,  // FOP_FSUB
    GEN7_ATOMIC_UNDEF                   = 0xFF
} GenAtomicOp;

extern const char* va_sub_names[26];

extern const char* pixel_size_str[2];

extern const char* lbp_creation_mode[3];

extern const char* avs_control_str[4];

extern const char* avs_exec_mode[3];

extern const char* mmf_exec_mode[4];

extern const char* mmf_enable_mode[3];

extern const char* ed_exec_mode[4];

extern const char* conv_exec_mode[4];

extern unsigned format_control_byteSize2[4];

extern unsigned ed_exec_mode_byte_size[4];

extern unsigned conv_exec_mode_size[4];

extern unsigned mmf_exec_mode_size[4];

extern unsigned lbp_creation_exec_mode_size[3];

extern unsigned lbp_correlation_mode_size[3];

extern unsigned mmf_exec_mode_bit_size[4];

extern unsigned output_format_control_size[4];

#define HASH_TABLE_SIZE 59

typedef struct
{
    PreDefined_Vars id;
    VISA_Type type;
    unsigned char majorVersion; // CISA major version when this becomes available
    bool isInR0;  // whether the variable value is stored in r0 or is
                  // appended after kernel input
    short byteOffset;  // byte offset of the variable's value
    unsigned num_elements;
    const char* str;
} CISA_PreDefined_Var_Info;

namespace vISA
{
    enum class SFID
    {
        NULL_SFID  =  0,
        SAMPLER    =  2,
        GATEWAY    =  3,
        DP_DC2     =  4,
        DP_WRITE   =  5, //DATAPORT WRITE
        URB        =  6, //URB
        SPAWNER    =  7, //THREAD SPAWNER
        VME        =  8, //VIDEO MOTION ESTIMATION
        DP_CC      =  9, //CONSTANT CACHE DATAPORT
        DP_DC0     = 10, //DATA CACHE DATAPORT
        DP_PI      = 11, //PIXEL INTERPOLATOR
        DP_DC1     = 12, //DATA CACHE DATAPORT1
        CRE        = 13, //CHECK & REFINEMENT ENGINE
        BTD        = 16, // bindless thread dispatcher
        RTHW       = 17, // ray trace HW accelerator
        TGM        = 18, // typed global memory
        SLM        = 19, // untyped shared local memory
        UGM        = 20, // untyped global memory
        UGML       = 21, // untyped global memory (low bandwidth)
    };

    inline int SFIDtoInt(SFID id)
    {
        if (id == SFID::BTD)
        {
            return 0x7;
        }
        else if (id == SFID::RTHW)
        {
            return 0x8;
        }
        else if (id == SFID::TGM)
        {
            return 0xD;
        }
        else if (id == SFID::SLM)
        {
            return 0xE;
        }
        else if (id == SFID::UGM)
        {
            return 0xF;
        }
        return static_cast<int>(id);
    };

    inline SFID intToSFID(int id, TARGET_PLATFORM platform)
    {
        if (platform >= Xe_DG2)
        {
            switch (id)
            {
                case 0x7:
                    return SFID::BTD;
                case 0x8:
                    return SFID::RTHW;
                case 0xD:
                    return SFID::TGM;
                case 0xE:
                    return SFID::SLM;
                case 0xF:
                    return SFID::UGM;
                default:
                    // fall through
                    break;
            }
        }
        return static_cast<SFID>(id);
    };
    inline SFID LSC_SFID_To_SFID(LSC_SFID lscId)
    {
        switch (lscId) {
        case LSC_UGM:  return SFID::UGM;
        case LSC_UGML: return SFID::UGML;
        case LSC_TGM:  return SFID::TGM;
        case LSC_SLM:  return SFID::SLM;
        default:
            assert(false && "invalid SFID for untyped LSC message");
            return SFID::NULL_SFID;
        }
    };
};

typedef enum
{
    PREDEF_SURF_0 = 254,
    PREDEF_SURF_1 = 1,
    PREDEF_SURF_2 = 2,
    PREDEF_SURF_3 = 3,
    PREDEF_SURF_1_OLD = 243,
    PREDEF_SURF_2_OLD = 244,
    PREDEF_SURF_3_OLD = 245,
    PREDEF_SURF_252 = 252, // bindless surfaces
    PREDEF_SURF_253 = 253,  // this is only used internally and should not be set by the user
    PREDEF_SURF_255 = 255
} PREDEFINED_SURF;

typedef struct
{
    int vISAId;  // their id in vISA binary (0-5)
    PREDEFINED_SURF genId;
    const char* name;  // name in vISA asm
} vISAPreDefinedSurface;

extern vISAPreDefinedSurface vISAPreDefSurf[COMMON_ISA_NUM_PREDEFINED_SURF_VER_3_1];

// bindless sampler field
const int BINDLESS_SAMPLER_ID = 31;
static const char* BINDLESS_SAMPLER_NAME = "S31";

const char* getSampleOp3DName(VISASampler3DSubOpCode opcode, TARGET_PLATFORM platform);
VISASampler3DSubOpCode getSampleOpFromName(const char *str, TARGET_PLATFORM platform);

/// ChannelMask - Channel mask used in vISA builder.
/// NOTE: This class is added to discourage developers to directly manipulate
/// the enumeration values. Instead, developers are encouraged to use interfaces
/// provides in this class to access channel mask.
class ChannelMask {
public:
  enum Encoding {   // ABGR BITCNT
    NOMASK  = 0x0,  // 0000      0
    R       = 0x1,  // 0001      1
    G       = 0x2,  // 0010      1
    RG      = 0x3,  // 0011      2
    B       = 0x4,  // 0100      1
    RB      = 0x5,  // 0101      2
    GB      = 0x6,  // 0110      2
    RGB     = 0x7,  // 0111      3
    A       = 0x8,  // 1000      1
    RA      = 0x9,  // 1001      2
    GA      = 0xA,  // 1010      2
    RGA     = 0xB,  // 1011      3
    BA      = 0xC,  // 1100      2
    RBA     = 0xD,  // 1101      3
    GBA     = 0xE,  // 1110      3
    RGBA    = 0xF   // 1111      4
  };

private:
  Encoding Value;
  static const char *Names[];
  static const uint64_t BitCounts = 0x4332322132212110ULL;

  ChannelMask(Encoding val) : Value(val) {}

  /// needReverseMaskForBinary - Channel mask needs reverse during vISA binary
  /// encoding.
  static bool needReverseMaskForBinary(ISA_Opcode opc) {
    return false;
  }

public:

  /// createFromString() - Create channel mask from its string representation.
  /// If the given string is not an valid representation, NOMASK is returned
  /// and (optional) error is set.
    static ChannelMask createFromString(const char *name, int *pError = 0) {
        char upperName[16];
        size_t strSize = strlen(name);
        memcpy_s(upperName, 16, name, strSize);

        for (unsigned i = 0; i < strSize; ++i)
        {
            upperName[i] = static_cast<char>(std::toupper(name[i]));
        }
        upperName[strSize] = '\0';

        for (unsigned i = 1; i < 16; ++i)
        {
            if (strcmp((const char *)upperName, Names[i]) == 0) {
                if (pError)
                    *pError = 0;
                return ChannelMask(Encoding(i));
            }
        }
        if (pError)
            *pError = 1;
        return ChannelMask(NOMASK);
    }

  /// createFromAPI - Create channel mask from vISA API.
  static ChannelMask createFromAPI(VISAChannelMask mask) {
    return ChannelMask(Encoding(unsigned(mask)));
  }

  /// createFromBinary - Create channel mask from vISA binary.
  static ChannelMask createFromBinary(ISA_Opcode opc, unsigned mask) {
    if (needReverseMaskForBinary(opc))
      mask = ~mask;
    mask &= 0xF;
    return ChannelMask(Encoding(mask));
  }

  /// createAPIFromBinary - Create channel mask API enumeration from vISA binary.
  /// This is helper function to shortcut the translation from vISA binary to
  /// vISA API enumeration.
  static VISAChannelMask createAPIFromBinary(ISA_Opcode opc, unsigned mask) {
    if (needReverseMaskForBinary(opc))
      mask = ~mask;
    mask &= 0xF;
    return ChannelMask(Encoding(mask)).getAPI();
  }

  /// createFromSingleChannel - Create channel mask from source single channel
  /// enumeration.
  static ChannelMask createFromSingleChannel(VISASourceSingleChannel single) {
    switch (single) {
    case VISA_3D_GATHER4_CHANNEL_R: return ChannelMask(R);
    case VISA_3D_GATHER4_CHANNEL_G: return ChannelMask(G);
    case VISA_3D_GATHER4_CHANNEL_B: return ChannelMask(B);
    case VISA_3D_GATHER4_CHANNEL_A: return ChannelMask(A);
    }
    return ChannelMask(NOMASK);
  }

  /// getName - Get the specified channel mask's string representation.
  const char *getString() const {
    return Names[unsigned(Value)];
  }

  /// getAPI - Get enumeration value defined in vISA API.
  VISAChannelMask getAPI() const {
    return VISAChannelMask(unsigned(Value));
  }

  VISASourceSingleChannel convertToSrcChannel() const
  {
      switch (Value)
      {
         case R:
             return VISA_3D_GATHER4_CHANNEL_R;
         case G:
             return VISA_3D_GATHER4_CHANNEL_G;
         case B:
             return VISA_3D_GATHER4_CHANNEL_B;
         case A:
             return VISA_3D_GATHER4_CHANNEL_A;
         default:
             assert(false && "can't be converted to single channel");
             return VISA_3D_GATHER4_CHANNEL_R;
      }
  }

  /// getBinary() - Get vISA binary encoding.
  /// NOTE getBinary() is different from getHWEncoding() and should be only
  /// used to dump vISA binary.
  VISAChannelMask getBinary(ISA_Opcode opc) const {
    unsigned mask = unsigned(Value);
    if (needReverseMaskForBinary(opc))
      mask = ~mask;
    mask &= 0xF;
    return VISAChannelMask(mask);
  }

  unsigned getBinary(ISA_Opcode opc, CHANNEL_OUTPUT_FORMAT out) const {
    unsigned mask = unsigned(Value);
    if (needReverseMaskForBinary(opc))
    {
      mask = ~mask;
    }
    mask &= 0xF;
    if ( opc != ISA_SAMPLE_UNORM )
    {
        return VISAChannelMask(mask);
    }
    return VISAChannelMask(mask) | ( (out << 4) & 0x30);
  }

  static unsigned getChannelOutputFormat(uint8_t channelOutput)
  {
      return (unsigned)((channelOutput >> 4) & 0x3);
  }

  /// getHWEncoding - Get HW encoding of channel mask. HW
  /// defines channel mask in negative logic to help remove header when all
  /// channels are enabled.
  unsigned getHWEncoding() const {
    return ~unsigned(Value) & 0xF;
  }

  /// getNumChannles - Get number of channels enabled.
  unsigned getNumEnabledChannels() const {
    return (BitCounts >> (unsigned(Value) * 4)) & 0xF;
  }

  /// getSingleChannel() - Get the single source channel enumeration. If more
  /// than one channels are enabled, only the lowest one is returned, e.g. if R
  /// and G are enabled, getSingleChannel() will return
  /// VISA_3D_GATHER4_CHANNEL_R only. It's developer's response to ensure only
  /// one channel is enabled if ChannelMask is willing to be converted into
  /// single source channel.
  VISASourceSingleChannel getSingleChannel() const {
    unsigned mask = unsigned(Value);
    if (mask & unsigned(R))
      return VISA_3D_GATHER4_CHANNEL_R;
    if (mask & unsigned(G))
      return VISA_3D_GATHER4_CHANNEL_G;
    if (mask & unsigned(B))
      return VISA_3D_GATHER4_CHANNEL_B;
    if (mask & unsigned(A))
      return VISA_3D_GATHER4_CHANNEL_A;
    return VISASourceSingleChannel(~0);
  }

  /// Comparison operator -
  bool operator==(const ChannelMask &other) const {
    return Value == other.Value;
  }

  /// Comparison operator -
  bool operator==(Encoding other) const {
    return Value == other;
  }
};

struct VISAFenceMask
{
    uint8_t commitEnable : 1;
    uint8_t flushICache : 1;
    uint8_t flushSCache : 1;
    uint8_t flushCCache : 1;
    uint8_t flushRWCache : 1;
    uint8_t isGlobal : 1;
    uint8_t flushL1Cache : 1;
    uint8_t SWFence: 1;
};

struct VISA3DSamplerOp
{
    VISASampler3DSubOpCode opcode;
    bool pixelNullMask;
    bool cpsEnable;
    bool nonUniformSampler;

    template <class T>
    static VISA3DSamplerOp extractSamplerOp(T val)
    {

        if (std::is_same<T, uint8_t>::value) {
            // Bit 0-4: subOpcode
            // Bit 5  : pixelNullMask
            // Bit 6  : cpsEnable
            // Bit 7  : non-uniform sampler
            VISA3DSamplerOp op;
            op.pixelNullMask = (val & (1 << 5)) != 0;
            op.cpsEnable = (val & (1 << 6)) != 0;
            op.nonUniformSampler = (val & (1 << 7)) != 0;
            // val & 0b00011111
            op.opcode = static_cast<VISASampler3DSubOpCode>(val & 0x1F);
            return op;
        }

        // Bit 0-7: subOpcode
        // Bit 8  : pixelNullMask
        // Bit 9  : cpsEnable
        // Bit 10 : non-uniform sampler
        VISA3DSamplerOp op;
        op.pixelNullMask = (val & (1 << 8)) != 0;
        op.cpsEnable = (val & (1 << 9)) != 0;
        op.nonUniformSampler = (val & (1 << 10)) != 0;
        // val & 0b01111111
        op.opcode = static_cast<VISASampler3DSubOpCode>(val & 0xFF);
        return op;
    }
};
namespace vISA{
    class Mem_Manager;
}

extern int processCommonISAHeader(common_isa_header& cisaHdr, unsigned& byte_pos, const void* isaBuffer, vISA::Mem_Manager* mem);

/// Use the following lengthOf macro ONLY for fixed size arrays (no pointers).
#define lengthOf(a) (sizeof(a)/sizeof(a[0]))

#endif /* COMMON_ISA_OPCODE_INCLUDED */
