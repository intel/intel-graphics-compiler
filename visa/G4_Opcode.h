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

#ifndef _G4_OPCODE_H_
#define _G4_OPCODE_H_
#include "visa_igc_common_header.h"
#include "common.h"

#define G4_MAX_SRCS       4
#define G4_DEFAULT_GRF_NUM  128

#define UNDEFINED_VAL   0xFFFFFFFF
#define UNDEFINED_SHORT 0x8000
#define UNDEFINED_EXEC_SIZE 0xFF

#define G4_BSIZE 1            // 1 byte 8 bits
#define G4_WSIZE 2            // 2 bytes 16 bits
#define G4_DSIZE 4            // 4 bytes 32 bits
#define IS_FTYPE(x) ((x) == Type_F)
#define IS_HFTYPE(x) ((x) == Type_HF)
#define IS_DFTYPE(x) ((x) == Type_DF || (x) == Type_NF)
#define IS_DTYPE(x) ((x) == Type_D || (x) == Type_UD)
#define IS_VINTTYPE(x) ((x) == Type_V || (x) == Type_UV)
#define IS_VFTYPE(x) ((x) == Type_VF)
#define IS_VTYPE(x) (IS_VINTTYPE(x) || IS_VFTYPE(x))
#define IS_WTYPE(x) ((x) == Type_W || (x) == Type_UW || (x) == Type_HF)
#define IS_BTYPE(x) ((x) == Type_B || (x) == Type_UB)
#define IS_QTYPE(x) ((x) == Type_Q || (x) == Type_UQ)
#define IS_SIGNED_INT(x) ((x) == Type_B || (x) == Type_W || (x) == Type_D || (x) == Type_Q)
#define IS_UNSIGNED_INT(x) ((x) == Type_UB || (x) == Type_UW || (x) == Type_UD || (x) == Type_UQ)
#define IS_INT(x) (IS_SIGNED_INT(x) || IS_UNSIGNED_INT(x))
#define IS_TYPE_INT(type)        (IS_SIGNED_INT(type) || IS_UNSIGNED_INT(type))
#define IS_TYPE_F32_F64(type)         (type == Type_F ||type == Type_DF || type == Type_NF)
#define IS_TYPE_FLOAT_ALL(type)     (type == Type_F ||type == Type_DF || type == Type_HF || type == Type_NF)
// added defs for CISA inst translation

#define GENX_GEN8P_MAX_WIDTH      64  // #

#define GENX_DATAPORT_IO_SZ       8   // # of dwords in read/write control area
#define GENX_SAMPLER_IO_SZ        8   // # of control dwords for Sampling Engine unit

#define G4_MAX_ADDR_IMM        511
#define G4_MIN_ADDR_IMM        -512

#define IVB_MSG_TYPE_OFFSET    14
#define MSG_BLOCK_SIZE_OFFSET   8
#define MSG_BLOCK_NUMBER_OFFSET 10
#define MAX_SEND_RESP_LEN    8
#define MAX_SEND_MESG_LEN    15
#define ADDR_REG_TYPE        Type_UW

#include "VISADefines.h"


// ToDo: move them to common.h?
#define MAKE_ENUM(X) X,
#define STRINGIFY(X) #X,

/*
 * For Gen6, only the following instructions can have
 * interger sources and float destination:
 * MOV, ADD, MUL, MAC, MAD, LINE
 */
#define Opcode_int_src_float_dst_OK(opc)        \
                                 ((opc == G4_mov)  || \
                                  (opc == G4_add)  || \
                                  (opc == G4_mul)  || \
                                  (opc == G4_mac)  || \
                                  (opc == G4_mad)  || \
                                  (opc == G4_line) || \
                                  (opc == G4_send) || \
                                  (opc == G4_sendc)|| \
                                  (opc == G4_sendsc) || \
                                  (opc == G4_sends))


#define Opcode_define_cond_mod(opc)        \
                                 ((opc == G4_add)  || \
                                  (opc == G4_mul)  || \
                                  (opc == G4_addc)  || \
                                  (opc == G4_cmp)  || \
                                  (opc == G4_cmpn)  || \
                                  (opc == G4_and)  || \
                                  (opc == G4_or)  || \
                                  (opc == G4_xor) || \
                                  (opc == G4_not)  || \
                                  (opc == G4_asr)  || \
                                  (opc == G4_avg)  || \
                                  (opc == G4_smov)  || \
                                  (opc == G4_dp2)  || \
                                  (opc == G4_dp3)  || \
                                  (opc == G4_dp4)  || \
                                  (opc == G4_dph)  || \
                                  (opc == G4_frc)  || \
                                  (opc == G4_line)  || \
                                  (opc == G4_lzd)  || \
                                  (opc == G4_fbh)  || \
                                  (opc == G4_fbl)  || \
                                  (opc == G4_cbit)  || \
                                  (opc == G4_lrp)  || \
                                  (opc == G4_mac)  || \
                                  (opc == G4_mad)  || \
                                  (opc == G4_mov)  || \
                                  (opc == G4_movi)  || \
                                  (opc == G4_pln)  || \
                                  (opc == G4_rndd)  || \
                                  (opc == G4_rndu)  || \
                                  (opc == G4_rnde)  || \
                                  (opc == G4_rndz)  || \
                                  (opc == G4_sad2)  || \
                                  (opc == G4_sada2)  || \
                                  (opc == G4_shl)  || \
                                  (opc == G4_shr)  || \
                                  (opc == G4_subb) || \
                                  (opc == G4_pseudo_mad ))

#define Opcode_can_use_cond_mod(opc)        \
                                 (opc == G4_sel)

enum G4_Align
{
    Either = 1,          // either
    Even = 2,            // even align
    Odd = 3,             // old align
    Even2GRF = 4,        // 2GRF even align 1100
    Odd2GRF = 5,          // 2GRF old align, 0011
    Align_NUM = 6        // Num of alignment
};

// To support sub register alignment
enum G4_SubReg_Align
{
    Any = 1,
    Even_Word = 2,
    Four_Word = 4,
    Eight_Word = 8,
    Sixteen_Word = 16,        // one register align
};


enum G4_SrcModifier
{
    Mod_Minus = 0,        // "-", negative
    Mod_Abs,            // (abs), absolute value
    Mod_Minus_Abs,        // -(abs)
    Mod_Not,            // invert (for BDW logic instruction)
    Mod_src_undef        // undefined
};

enum G4_CondModifier
{
    Mod_z = 0,            // zero
    Mod_e,                // equal
    Mod_nz,                // not zero
    Mod_ne,                // not equal
    Mod_g,                // greater
    Mod_ge,                // greater or equal
    Mod_l,                // less
    Mod_le,                // less or equal
    Mod_o,                // overflow
    Mod_r,                // round increment
    Mod_u,                // unorder (NaN)
    Mod_cond_undef        // undefined
};

enum G4_PredState
{
    PredState_Plus = 0, // +
    PredState_Minus,    // -
    PredState_undef     // undefined
};

enum G4_RegAccess {
    Direct,
    IndirGRF,
};

//
// register and Imm data type
// Note: Check G4_Type_ByteFootprint if this is modified
//
enum G4_Type
{
    Type_UD = 0,// unsigned double word integer
    Type_D,        // signed double word integer
    Type_UW,    // unsigned word integer
    Type_W,        // signed word integer
    Type_UB,    // unsigned byte integer
    Type_B,        // signed byte integer
    Type_F,        // signed single precision
    Type_VF,    // 32-bit restricted Vector Float
    Type_V,     // 32-bit halfbyte integer Vector
    Type_DF,
    Type_BOOL,
    Type_UV,
    Type_Q,     // 64-bit signed integer
    Type_UQ,    // 64-bit unsigned integer
    Type_HF,    // half float
    Type_NF,    // native float (only used by plane macro)
    Type_UNDEF
};

typedef struct
{
    G4_Type type;
    unsigned int bitSize;
    unsigned int byteSize;
    unsigned short footprint; // bit pattern that corresponds to type's byte usage
    const char* str; //constant string representation of the type
} G4_Type_Info;

extern G4_Type_Info G4_Type_Table[Type_UNDEF+1];

enum G4_InstType
{
    InstTypeMov        = 0,    // mov or sel
    InstTypeLogic,            // not, and, or, xor, ...
    InstTypeCompare,        // cmp, cmpn
    InstTypeFlow,            // if, iff, do, while, break, ...
    InstTypeMask,            // msave, mrest, push, pop
    InstTypeArith,            // add, mul, frc, mac , ...
    InstTypeVector,            // sad, sad2, dp4, dp3, dp2, ..
    InstTypeMisc,            // send, wait, nop, illegal
    InstTypePseudoLogic,    // Pseudo_not, Pseudo_and, ...
    InstTypeReserved        // reserved (unused)
};

enum G4_RegFileKind
{
    G4_UndefinedRF = 0x0,
    G4_GRF        = 0x1,            // general register file
    G4_ADDRESS = 0x2,            // architectural register file
    G4_INPUT    = 0x4,            // input payload register
    G4_FLAG        = 0x20,
};

//
// multiple options can coexist so we define one bit for each option
//

enum G4_InstOption
{
    InstOpt_NoOpt       = 0x0,
    InstOpt_Align16     = 0x00000002,
    InstOpt_M0          = 0x00100000,
    InstOpt_M4          = 0x00200000,
    InstOpt_M8          = 0x00400000,
    InstOpt_M12         = 0x00800000,
    InstOpt_M16         = 0x01000000,
    InstOpt_M20         = 0x02000000,
    InstOpt_M24         = 0x04000000,
    InstOpt_M28         = 0x08000000,
    InstOpt_Switch      = 0x00000010,
    InstOpt_Atomic      = 0x00000020,
    InstOpt_NoDDChk     = 0x00000040,
    InstOpt_NoDDClr     = 0x00000080,
    InstOpt_WriteEnable = 0x00000100,

    InstOpt_BreakPoint  = 0x00000200,
    InstOpt_EOT         = 0x00000400,
    InstOpt_AccWrCtrl   = 0x00000800,
    InstOpt_NoCompact   = 0x00001000,
    InstOpt_Compacted   = 0x00002000,
    InstOpt_NoSrcDepSet = 0x00004000,
    InstOpt_NoPreempt   = 0x00008000,

    InstOpt_END         = 0xFFFFFFFF
};


#define InstOpt_QuarterMasks \
    (InstOpt_M0 | InstOpt_M4 | InstOpt_M8 | InstOpt_M12 | InstOpt_M16 | InstOpt_M20 | InstOpt_M24 | InstOpt_M28)
#define InstOpt_Masks (InstOpt_QuarterMasks | InstOpt_WriteEnable)

typedef struct _G4_InstOptInfo
{
    G4_InstOption optMask;
    const char*   optStr;
} G4_InstOptInfo;

extern G4_InstOptInfo InstOptInfo[];

//various attributes for the Gen opcodes
#define ATTR_COMMUTATIVE        0x00000010
#define ATTR_FLOAT_SRC_ONLY     0x00000040
#define ATTR_NONE               0x00000000

#define INST_COMMUTATIVE(inst)      (G4_Inst_Table[inst].attributes & ATTR_COMMUTATIVE)
#define INST_FLOAT_SRC_ONLY(inst)   (G4_Inst_Table[inst].attributes & ATTR_FLOAT_SRC_ONLY)

#define         GENX_MAX_H_STRIDE           4

#define HANDLE_INST( op, nsrc, ndst, type, plat, attr ) G4_ ## op,
enum G4_opcode
{
#include "G4Instruction.def"
    G4_NUM_OPCODE
};


typedef struct _G4_Inst_Info
{
    G4_opcode   op;       // just for debugging purpose
    const char* str;      // for emitting asm code
    uint8_t     n_srcs;   // # of source operands
    uint8_t     n_dst;    // # of dst operands
    G4_InstType instType; // inst classification
    TARGET_PLATFORM    platform; // The platform that first supports this inst.
    int         attributes;
} G4_Inst_Info;

extern G4_Inst_Info G4_Inst_Table[G4_NUM_OPCODE];

// Relation between two regions. The comparison is based on memory locations
// regions reference, which are described by the left-bound, the right bound,
// and their footprint vectors. The exact order of elements or type are not
// considered. For example, region comparison algorithm will return Rel_eq for
// the following region pairs:
//
// * (8) V(0,0)<1>:d and (8) V(0,0)<1;4,2>:d
// * (8) V(0,0)<1>:d and (16) V(0,0)<1;1,0>:w
// * (8) V(0,0)<1>:d and (16) V(0,0)<0;8,1>:d
//
enum G4_CmpRelation
{
    Rel_eq = 0,        // region A is the same as region B
    Rel_lt,            // region A is a subset of region B
    Rel_gt,            // region A is a super set of region B
    Rel_interfere,     // region A overlaps with region B
    Rel_disjoint,      // region A and region B are disjoint
    Rel_undef          // region A and region B have different base variable
};

#define OPND_NUM_ENUM(DO) \
    DO(Opnd_dst) \
    DO(Opnd_src0) \
    DO(Opnd_src1) \
    DO(Opnd_src2) \
    DO(Opnd_src3) \
    DO(Opnd_pred) \
    DO(Opnd_condMod) \
    DO(Opnd_implAccSrc) \
    DO(Opnd_implAccDst) \
    DO(Opnd_total_num)

enum Gen4_Operand_Number
{
    OPND_NUM_ENUM(MAKE_ENUM)
};

enum G4_ArchRegKind {
    // Note that the enum values are different from HW values,
    // as we represent acc0 and acc1 as different AReg for example
    AREG_NULL = 0, // null register
    AREG_A0,          // address register
    AREG_ACC0,        // accumulator register
    AREG_ACC1,        // accumulator register
    AREG_MASK0,       // mask register
    AREG_MS0,         // mask stack register
    AREG_DBG,         // mask stack depth register
    AREG_SR0,         // state register
    AREG_CR0,         // control register
    AREG_N0,          // notification count register
    AREG_N1,          // notification count register
    AREG_IP,          // instruction pointer register
    AREG_F0,          // flag register
    AREG_F1,          // flag register
    AREG_TM0,         // timestamp register
    AREG_TDR0,        // TDR register
    AREG_SP,          // SP register
    AREG_LAST
};

enum G4_AccRegSel
{
    ACC2 = 0,
    ACC3,
    ACC4,
    ACC5,
    ACC6,
    ACC7,
    ACC8,
    ACC9,
    NOACC,
    ACC_UNDEFINED = 0xff
};

// global functions
inline unsigned int getNumAddrRegisters(void) { return 16; }
extern short Operand_Type_Rank(G4_Type type);
uint8_t roundDownPow2(uint8_t n);
bool isPow2(uint8_t n);
inline uint32_t getTypeSize(G4_Type ty) { return G4_Type_Table[ty].byteSize; }
inline bool isLowPrecisionFloatTy(G4_Type ty)
{
    return ty == Type_HF;
}

#define SUB_ALIGNMENT_GRFALIGN (Sixteen_Word)
#define SUB_ALIGNMENT_HALFGRFALIGN (Eight_Word)
#endif  // _G4_OPCODE_H_
