/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _G4_OPCODE_H_
#define _G4_OPCODE_H_
#include "Assertions.h"
#include "common.h"
#include "visa_igc_common_header.h"

#define G4_MAX_SRCS 4
#define G4_MAX_INTRINSIC_SRCS 8
#define UNDEFINED_VAL 0xFFFFFFFF
#define UNDEFINED_SHORT 0x8000
#define UNDEFINED_EXEC_SIZE 0xFF

#define G4_WSIZE 2 // 2 bytes 16 bits
#define G4_DSIZE 4 // 4 bytes 32 bits
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
#define IS_SIGNED_INT(x)                                                       \
  ((x) == Type_B || (x) == Type_W || (x) == Type_D || (x) == Type_Q)
#define IS_UNSIGNED_INT(x)                                                     \
  ((x) == Type_UB || (x) == Type_UW || (x) == Type_UD || (x) == Type_UQ)
#define IS_INT(x) (IS_SIGNED_INT(x) || IS_UNSIGNED_INT(x))
#define IS_TYPE_INT(type) (IS_SIGNED_INT(type) || IS_UNSIGNED_INT(type))
#define IS_TYPE_F32_F64(type)                                                  \
  (type == Type_F || type == Type_DF || type == Type_NF)
#define IS_TYPE_FLOAT_ALL(type)                                                \
  (type == Type_F || type == Type_DF || type == Type_HF || type == Type_NF ||  \
   type == Type_BF)
#define IS_TYPE_FLOAT_FOR_ACC(type)                                            \
  (type == Type_F || type == Type_DF || type == Type_HF)
#define IS_TYPE_LONG(type)                                                     \
  (type == Type_DF || type == Type_UQ || type == Type_Q)
#define IS_TYPE_INTEGER(type)                                                  \
  (type == Type_UW || type == Type_W || type == Type_B || type == Type_UB ||   \
   type == Type_V || type == Type_UV || type == Type_UD || type == Type_D)

#define ADDR_REG_TYPE Type_UW

// ToDo: move them to common.h?
#define MAKE_ENUM(X) X,
#define STRINGIFY(X) #X,

enum class BankAlign {
  Either = 1,   // either
  Even = 2,     // even align
  Odd = 3,      // old align
  Even2GRF = 4, // 2-GRF even align 1100
  Odd2GRF = 5,  // 2-GRF old align, 0011
  Align_NUM = 6 // Num of alignment
};

// An instruction's execution width
struct G4_ExecSize {
  unsigned char value;

  // goal is to keep constructors "explicit" so they
  // better distingushed in parameter lists for overload resolution
  explicit constexpr G4_ExecSize(unsigned char _value) : value(_value) {}
  // we could provide a non-const version of these that asserts on SIMD sizes
  explicit G4_ExecSize(int _value) : value((unsigned)_value) {}
  explicit G4_ExecSize(unsigned int _value) : value(_value) {}
  // the default constructor can be implicit
  constexpr G4_ExecSize() : value(0) {}

  G4_ExecSize(const G4_ExecSize &) = default;
  G4_ExecSize &operator=(const G4_ExecSize &) = default;

  G4_ExecSize &operator*=(unsigned char s) {
    value *= s;
    return *this;
  }
  G4_ExecSize &operator/=(unsigned char s) {
    value /= s;
    return *this;
  }

  bool operator<(G4_ExecSize rhs) const { return value < rhs.value; }
  bool operator<=(G4_ExecSize rhs) const { return value <= rhs.value; }
  bool operator==(G4_ExecSize rhs) const { return value == rhs.value; }
  bool operator!=(G4_ExecSize rhs) const { return value != rhs.value; }
  bool operator>=(G4_ExecSize rhs) const { return value >= rhs.value; }
  bool operator>(G4_ExecSize rhs) const { return value > rhs.value; }

  bool operator<(unsigned rhs) const { return value < (unsigned char)rhs; }
  bool operator<=(unsigned rhs) const { return value <= (unsigned char)rhs; }
  bool operator==(unsigned rhs) const { return value == (unsigned char)rhs; }
  bool operator!=(unsigned rhs) const { return value != (unsigned char)rhs; }
  bool operator>=(unsigned rhs) const { return value >= (unsigned char)rhs; }
  bool operator>(unsigned rhs) const { return value > (unsigned char)rhs; }

  bool operator<(int rhs) const { return value < (unsigned char)rhs; }
  bool operator<=(int rhs) const { return value <= (unsigned char)rhs; }
  bool operator==(int rhs) const { return value == (unsigned char)rhs; }
  bool operator!=(int rhs) const { return value != (unsigned char)rhs; }
  bool operator>=(int rhs) const { return value >= (unsigned char)rhs; }
  bool operator>(int rhs) const { return value > (unsigned char)rhs; }

  operator unsigned char() const { return value; }
};
namespace g4 {
constexpr G4_ExecSize SIMD1((unsigned char)1);
constexpr G4_ExecSize SIMD2((unsigned char)2);
constexpr G4_ExecSize SIMD4((unsigned char)4);
constexpr G4_ExecSize SIMD8((unsigned char)8);
constexpr G4_ExecSize SIMD16((unsigned char)16);
constexpr G4_ExecSize SIMD32((unsigned char)32);
// TODO: remove/merge with G4_ExecSize(0) uses
constexpr G4_ExecSize SIMD_UNDEFINED((unsigned char)UNDEFINED_EXEC_SIZE);
} // namespace g4

namespace g4 {
template <typename T> static inline T alignUp(T a, T n) {
  return ((n + a - 1) - (n + a - 1) % a);
}
} // namespace g4

// saturation
// (typesafe enum value with operators and conversions)
//
// use g4::SAT or g4::NOSAT to reference the two values
struct G4_Sat {
  const enum class Value { NOSAT = 0, SAT } value;
  constexpr G4_Sat(Value _value) : value(_value) {}
  operator bool() const { return value == Value::SAT; }
};
namespace g4 {
// enables g4::SAT as a symbol for a short-hand saturation
constexpr G4_Sat SAT(G4_Sat::Value::SAT);
constexpr G4_Sat NOSAT(G4_Sat::Value::NOSAT);
} // namespace g4

// To support sub register alignment
enum G4_SubReg_Align {
  Any = 1,
  Even_Word = 2,
  Four_Word = 4,
  Eight_Word = 8,
  Sixteen_Word = 16,
  ThirtyTwo_Word = 32
};

enum G4_SrcModifier : unsigned char {
  Mod_Minus = 0, // "-", negative
  Mod_Abs,       // (abs), absolute value
  Mod_Minus_Abs, // -(abs)
  Mod_Not,       // invert (for BDW logic instruction)
  Mod_src_undef  // undefined
};

enum G4_CondModifier : unsigned char {
  Mod_z = 0,     // zero
  Mod_e,         // equal
  Mod_nz,        // not zero
  Mod_ne,        // not equal
  Mod_g,         // greater
  Mod_ge,        // greater or equal
  Mod_l,         // less
  Mod_le,        // less or equal
  Mod_o,         // overflow
  Mod_r,         // round increment
  Mod_u,         // unorder (NaN)
  Mod_cond_undef // undefined
};

enum G4_PredState : unsigned char {
  PredState_Plus = 0, // +
  PredState_Minus,    // -
  PredState_undef     // undefined
};

enum G4_RegAccess : unsigned char {
  Direct,
  IndirGRF,
};

//
// register and Imm data type
// Note: Check G4_Type_ByteFootprint if this is modified
//
enum G4_Type : unsigned char {
  Type_UD = 0, // unsigned double word integer
  Type_D,      // signed double word integer
  Type_UW,     // unsigned word integer
  Type_W,      // signed word integer
  Type_UB,     // unsigned byte integer
  Type_B,      // signed byte integer
  Type_F,      // signed single precision
  Type_VF,     // 32-bit restricted Vector Float
  Type_V,      // 32-bit halfbyte integer Vector
  Type_DF,
  Type_BOOL,
  Type_UV,
  Type_Q,  // 64-bit signed integer
  Type_UQ, // 64-bit unsigned integer
  Type_HF, // half float
  Type_NF, // native float (only used by plane macro)
  Type_BF, // bfloat16 (used in mov only)
  Type_UNDEF
};

typedef struct {
  G4_Type type;
  unsigned char bitSize;
  unsigned char byteSize;
  unsigned char footprint; // bit pattern that corresponds to type's byte usage
  const char *syntax;      // constant string representation of the type
} G4_Type_Info;

constexpr G4_Type_Info G4_Type_Table[Type_UNDEF + 1]{
    {Type_UD, 32, 4, 0x0F, "ud"},
    {Type_D, 32, 4, 0x0F, "d"},
    {Type_UW, 16, 2, 0x03, "uw"},
    {Type_W, 16, 2, 0x03, "w"},
    {Type_UB, 8, 1, 0x01, "ub"},
    {Type_B, 8, 1, 0x01, "b"},
    {Type_F, 32, 4, 0x0F, "f"},
    {Type_VF, 32, 4, 0x0F, "vf"}, // handle as F?
    {Type_V, 32, 4, 0x0F, "v"},   // handle as D?
    {Type_DF, 64, 8, 0xFF, "df"},
    {Type_BOOL, 1, 2, 0x01, "bool"}, // TODO: how to decide 1 bit here?
    {Type_UV, 32, 4, 0x0F, "uv"},
    {Type_Q, 64, 8, 0xFF, "q"},
    {Type_UQ, 64, 8, 0xFF, "uq"},
    {Type_HF, 16, 2, 0x03, "hf"},
    {Type_NF, 64, 8, 0xFF, "nf"},
    {Type_BF, 16, 2, 0x03, "bf"},
    {Type_UNDEF, 0, 0, 0x0, "???"}};
static inline constexpr G4_Type_Info TypeInfo(G4_Type t) {
  return G4_Type_Table[(unsigned)t > (unsigned)Type_UNDEF ? Type_UNDEF : t];
}
static inline constexpr unsigned short TypeSize(G4_Type t) {
  return TypeInfo(t).byteSize;
}
static inline constexpr unsigned short TypeBitSize(G4_Type t) {
  // TODO: can we use TypeBitSize/8 here? Need to determine why bool is 2 B
  return TypeInfo(t).bitSize;
}
static inline constexpr unsigned short TypeFootprint(G4_Type t) {
  return TypeInfo(t).footprint;
}
static inline constexpr const char *TypeSymbol(G4_Type t) {
  return TypeInfo(t).syntax;
}

enum G4_InstType {
  InstTypeMov = 0,     // mov or sel
  InstTypeLogic,       // not, and, or, xor, ...
  InstTypeCompare,     // cmp, cmpn
  InstTypeFlow,        // if, iff, do, while, break, ...
  InstTypeArith,       // add, mul, frc, mac , ...
  InstTypeVector,      // sad, sad2, dp4, dp3, dp2, ..
  InstTypeMisc,        // send, wait, nop, illegal
  InstTypePseudoLogic, // Pseudo_not, Pseudo_and, ...
  InstTypeReserved     // reserved (unused)
};

enum G4_RegFileKind : unsigned char {
  G4_UndefinedRF = 0x0,
  G4_GRF = 0x1,     // general register file
  G4_ADDRESS = 0x2, // architectural register file
  G4_INPUT = 0x4,   // input payload register
  G4_FLAG = 0x20,
  G4_SCALAR = 0x40,
};

//
// multiple options can coexist so we define one bit for each option
//
enum G4_InstOption {
  InstOpt_NoOpt = 0x0,
  InstOpt_Align16 = 0x00000002,
  InstOpt_M0 = 0x00100000,
  InstOpt_M4 = 0x00200000,
  InstOpt_M8 = 0x00400000,
  InstOpt_M12 = 0x00800000,
  InstOpt_M16 = 0x01000000,
  InstOpt_M20 = 0x02000000,
  InstOpt_M24 = 0x04000000,
  InstOpt_M28 = 0x08000000,
  InstOpt_Switch = 0x00000010,
  InstOpt_Atomic = 0x00000020,
  InstOpt_NoDDChk = 0x00000040,
  InstOpt_NoDDClr = 0x00000080,
  InstOpt_WriteEnable = 0x00000100,

  InstOpt_BreakPoint = 0x00000200,
  InstOpt_EOT = 0x00000400,
  InstOpt_AccWrCtrl = 0x00000800,
  InstOpt_NoCompact = 0x00001000,
  InstOpt_Compacted = 0x00002000,
  InstOpt_NoSrcDepSet = 0x00004000,
  InstOpt_NoPreempt = 0x00008000,
  InstOpt_Serialize = 0x00010000,
  InstOpt_CachelineAligned = 0x00040000,

  InstOpt_END = 0xFFFFFFFF
};

#define InstOpt_QuarterMasks                                                   \
  (InstOpt_M0 | InstOpt_M4 | InstOpt_M8 | InstOpt_M12 | InstOpt_M16 |          \
   InstOpt_M20 | InstOpt_M24 | InstOpt_M28)
#define InstOpt_Masks (InstOpt_QuarterMasks | InstOpt_WriteEnable)

// TODO: to a more proper data type
// ==> step 1: use uint32_t so all the untyped uses still compile
//             but replace all uint32_t (and other int types with G4_InstOpts)
//     step 2: make G4_InstOpts a typesafe enum;
//             #define old unscoped enum names to new typesafe enum
//             (define various set operators so |, &, and ~ still work)
//             remove the few magic number uses with valid type enumes
//     step 3: mechanically (find and replace) old unscoped enums with
//             typesafe symbols and remove #defines
using G4_InstOpts = uint32_t;

typedef struct _G4_InstOptInfo {
  G4_InstOption optMask;
  const char *optStr;
} G4_InstOptInfo;

// various attributes for the Gen opcodes
#define ATTR_NONE 0x00000000
#define ATTR_PSEUDO 0x00000001
#define ATTR_COMMUTATIVE 0x00000002
#define ATTR_FLOAT_SRC_ONLY 0x00000004
#define ATTR_WIDE_DST 0x00000008

#define INST_PSEUDO(inst) (G4_Inst_Table[inst].attributes & ATTR_PSEUDO)
#define INST_COMMUTATIVE(inst)                                                 \
  (G4_Inst_Table[inst].attributes & ATTR_COMMUTATIVE)
#define INST_FLOAT_SRC_ONLY(inst)                                              \
  (G4_Inst_Table[inst].attributes & ATTR_FLOAT_SRC_ONLY)
#define INST_WIDE_DST(inst) (G4_Inst_Table[inst].attributes & ATTR_WIDE_DST)

#define HANDLE_INST(op, nsrc, ndst, type, plat, attr) G4_##op,

enum G4_opcode {
#include "G4Instruction.h"
  G4_NUM_OPCODE
};

typedef struct _G4_Inst_Info {
  G4_opcode op;             // just for debugging purpose
  const char *str;          // for emitting asm code
  uint8_t n_srcs;           // # of source operands
  uint8_t n_dst;            // # of dst operands
  G4_InstType instType;     // inst classification
  TARGET_PLATFORM platform; // The platform that first supports this inst.
  int attributes;
} G4_Inst_Info;

// ToDo: make this into G4_INST so that not everyone and their grandma are
// directly reading it
extern const G4_Inst_Info G4_Inst_Table[G4_NUM_OPCODE + 1];

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
enum G4_CmpRelation {
  Rel_eq = 0,    // region A is the same as region B
  Rel_lt,        // region A is a subset of region B
  Rel_gt,        // region A is a super set of region B
  Rel_interfere, // region A overlaps with region B
  Rel_disjoint,  // region A and region B are disjoint
  Rel_undef      // region A and region B have different base variable
};

#define OPND_NUM_ENUM(DO)                                                      \
  DO(Opnd_dst)                                                                 \
  DO(Opnd_src0)                                                                \
  DO(Opnd_src1)                                                                \
  DO(Opnd_src2)                                                                \
  DO(Opnd_src3)                                                                \
  DO(Opnd_src4)                                                                \
  DO(Opnd_src5)                                                                \
  DO(Opnd_src6)                                                                \
  DO(Opnd_src7)                                                                \
  DO(Opnd_pred)                                                                \
  DO(Opnd_condMod)                                                             \
  DO(Opnd_implAccSrc)                                                          \
  DO(Opnd_implAccDst)                                                          \
  DO(Opnd_total_num)

enum Gen4_Operand_Number : unsigned char { OPND_NUM_ENUM(MAKE_ENUM) };

enum G4_ArchRegKind {
  // Note that the enum values are different from HW values,
  // as we represent acc0 and acc1 as different AReg for example
  AREG_NULL = 0, // null register
  AREG_A0,       // address register
  AREG_ACC0,     // accumulator register
  AREG_ACC1,     // accumulator register
  AREG_MASK0,    // mask register
  AREG_MSG0,     // message control register
  AREG_DBG,      // mask stack depth register
  AREG_SR0,      // state register
  AREG_CR0,      // control register
  AREG_N0,       // notification count register
  AREG_N1,       // notification count register
  AREG_IP,       // instruction pointer register
  AREG_F0,       // flag register
  AREG_F1,       // flag register
  AREG_TM0,      // timestamp register
  AREG_TDR0,     // TDR register
  AREG_SP,       // SP register
  AREG_F2,       // flag register (PVC+)
  AREG_F3,       // flag register (PVC+)
  AREG_LAST
};

enum G4_AccRegSel : unsigned char {
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

uint8_t roundDownPow2(uint8_t n);

// G4_type related global functions
inline bool isLowPrecisionFloatTy(G4_Type ty) {
  return ty == Type_HF || ty == Type_BF;
}

inline G4_Type floatToSameWidthIntType(G4_Type floatTy) {
  vASSERT(IS_TYPE_FLOAT_ALL(floatTy));
  switch (TypeSize(floatTy)) {
  case 1:
    return Type_UB;
  case 2:
    return Type_UW;
  case 4:
    return Type_UD;
  case 8:
    return Type_UQ;
  default:
    vISA_ASSERT_UNREACHABLE("illegal type size");
    return Type_UD;
  }
}

// size is the number of byte
inline G4_SubReg_Align Get_G4_SubRegAlign_From_Size(uint16_t size,
                                                    TARGET_PLATFORM platform,
                                                    G4_SubReg_Align GRFAlign) {
  switch (size) {
  case 1:
  case 2:
    return Any;
  case 4:
    return Even_Word;
  case 8:
    if (platform != GENX_BXT)
      return Four_Word;
    // FALL THROUGH
    // WA: It's a workaround where a potential HW issue needs
    // identifying.
  case 16:
    return Eight_Word;
  case 32:
    return Sixteen_Word;
  case 64:
    return ThirtyTwo_Word;
  default:
    return GRFAlign;
  }
}

inline G4_SubReg_Align Get_G4_SubRegAlign_From_Type(G4_Type ty) {
  switch (ty) {
  case Type_B:
  case Type_UB:
  case Type_W:
  case Type_UW:
    return Any;
  case Type_UD:
  case Type_D:
  case Type_F:
    return Even_Word;
  case Type_DF:
    return Four_Word;
  case Type_V:
  case Type_VF:
  case Type_UV:
    return Eight_Word;
  case Type_Q:
  case Type_UQ:
    return Four_Word;
  default:
    return Any;
  }
}
#endif // _G4_OPCODE_H_
