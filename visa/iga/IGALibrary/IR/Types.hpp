/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_IR_TYPES_HPP
#define IGA_IR_TYPES_HPP

// WARNING: the IR is subject to change without any notice.  External tools
// should use the official interfaces in the external API.  Those interfaces
// are tested between releases and maintained even with changes to the IR.


#include <cstdint>
#include <tuple>

#include "../EnumBitset.hpp"
#include "../api/iga_bxml_enums.hpp"
#include "../api/iga_types_ext.hpp"

namespace iga
{

// The GEN platform version
enum class Platform
{
// GEN version encoding. The version encoding must be the same as GEN_VER in iga.h
#define IGA_GEN_VER_ORDINAL(MAJ,MIN) ((MAJ)<<16)|(MIN)

// XE version encoding. The version encoding must be the same as XE_VER in iga.h
#define IGA_XE_VER_ORDINAL(XE,SUBVER) (((XE)<<24)|SUBVER)

    INVALID     = 0,

    GEN6        = IGA_GEN_VER_ORDINAL( 6, 0 ),
    GEN7        = IGA_GEN_VER_ORDINAL( 7, 0 ),
    GEN7P5      = IGA_GEN_VER_ORDINAL( 7, 5 ),
    GEN8        = IGA_GEN_VER_ORDINAL( 8, 0 ),
    GEN8LP      = IGA_GEN_VER_ORDINAL( 8, 1 ),
    GEN9        = IGA_GEN_VER_ORDINAL( 9, 0 ),
    GEN9LP      = IGA_GEN_VER_ORDINAL( 9, 1 ),
    GEN9P5      = IGA_GEN_VER_ORDINAL( 9, 5 ),
    GEN10       = IGA_GEN_VER_ORDINAL(10, 0 ),
    GEN11       = IGA_GEN_VER_ORDINAL(11, 0 ),
    // XE version
    XE          = IGA_XE_VER_ORDINAL(1, 0), // TGL
    XE_HP       = IGA_XE_VER_ORDINAL(1, 1),
    XE_HPG      = IGA_XE_VER_ORDINAL(1, 2),
    XE_HPC      = IGA_XE_VER_ORDINAL(1, 4), // XeHPC-XT, preserved (1, 3) for XeHPC-XL
    FUTURE      = 0x7FFFFFFF
#undef IGA_GEN_VER_ORDINAL
};

struct Predication
{
    PredCtrl  function;
    bool      inverse; // TODO: enum

    Predication() : function(PredCtrl::NONE), inverse(false) { }
    Predication(PredCtrl ctrl, bool inv) : function(ctrl), inverse(inv) { }
};


typedef BranchCtrl BranchCntrl; // for backwards compatibility

static inline int ExecSizeToInt(ExecSize es)
{
    return static_cast<int>(es);
}
static inline ExecSize ExecSizeFromInt(int es)
{
    return static_cast<ExecSize>(es);
}

// for math macro register access (madm, math.invm, and math.rsqrtm)
// e.g.  madm (8) r13.mme2:f  r14:mme7:f  r16:nomme ...
//                    ^^^^        ^^^^        ^^^^^
enum class MathMacroExt
{
    INVALID,
    MME0,    // encodes as 0000b
    MME1,    // encodes as 0001b
    MME2,    // encodes as 0010b
    MME3,    // encodes as 0011b
    MME4,    // encodes as 0100b
    MME5,    // encodes as 0101b
    MME6,    // encodes as 0110b
    MME7,    // encodes as 0111b
    NOMME,   // encodes as 1000b
};


// how much to shift <right,left> to get to from byte offset
// to subregister offset
//   I.e. subReg = (byteOff << left) >> right;
// this allows us to scale a subregister byte offset up OR down
static inline std::tuple<uint32_t, uint32_t>
    TypeSizeShiftsOffsetToSubreg(Type type)
{
    uint32_t shl = 0, shr = 0; // by default no scaling

    switch (type) {
    // subbyte types
    case Type::U1:
        shl = 3;
        break;
    case Type::U2:
    case Type::S2:
        shl = 2;
        break;
    case Type::U4:
    case Type::S4:
        shl = 1;
        break;

    case Type::QF:
    case Type::BF8:
        break;
    case Type::TF32:
        shr = 2;
        break;

    // 1-byte types
    case Type::UB:
    case Type::B:
        break;
    // 2-byte types
    case Type::UW:
    case Type::W:
    case Type::HF:
    case Type::BF:
        shr = 1;
        break;
    // 4-byte types
    case Type::UD:
    case Type::D:
    case Type::F:
    case Type::NF: // NF regions the same as F
        shr = 2;
        break;
    case Type::UQ:
    case Type::Q:
    case Type::DF:
        shr = 3;
        break;
    default: // invalid types
        break;
    }
    return std::make_tuple(shl,shr);
}

// e.g. Type::UD == 32
static inline uint32_t TypeSizeInBits(Type t)
{
    auto ti = TypeSizeShiftsOffsetToSubreg(t);
    return (8 << std::get<1>(ti)) >> std::get<0>(ti);
}
static inline uint32_t TypeSizeInBitsWithDefault(Type type, int dft = 0)
{
    return type == Type::INVALID ? dft : TypeSizeInBits(type);
}
static inline bool TypeIs64b(Type t)
{
    return TypeSizeInBitsWithDefault(t,0) == 64;
}
static inline bool TypeIsFloating(Type t)
{
    switch (t)
    {
    case Type::F:
    case Type::BF:
    case Type::QF:
    case Type::BF8:
    case Type::TF32:
    case Type::HF:
    case Type::DF:
    case Type::VF:
    case Type::NF:
        return true;
    default:
        return false;
    }
}
// static inline bool TypeIsSubByte(Type t) {
//    return std::get<1>(TypeSizeShiftsOffsetToSubreg(t)) > 0;
// }

struct Region {
    enum class Vert {
        VT_0       =  0,
        VT_1       =  1,
        VT_2       =  2,
        VT_4       =  4,
        VT_8       =  8,
        VT_16      = 16,
        VT_32      = 32,

        VT_VxH     = 31, // special VxH mode for indirect region
        VT_INVALID = 63,
    };
    enum class Width {
        WI_1       =  1,
        WI_2       =  2,
        WI_4       =  4,
        WI_8       =  8,
        WI_16      = 16,
        WI_INVALID = 31
    };
    enum class Horz {
        HZ_0       =  0, // not permitted on DstOps (unless MBZ)
        HZ_1       =  1,
        HZ_2       =  2,
        HZ_4       =  4,
        HZ_INVALID = 15
    };
    union {
        struct {
            unsigned int v : 6;
            unsigned int w : 5;
            unsigned int h : 4;
        };
        uint32_t bits;
    };

    constexpr void set(Vert _v, Width _w, Horz _h) {
        this->bits = 0; // clear padding
        v = static_cast<unsigned int>(_v);
        w = static_cast<unsigned int>(_w);
        h = static_cast<unsigned int>(_h);
    }

    constexpr void set(Vert vt) {
        v = static_cast<unsigned int>(vt);
    }
    constexpr void set(Width wi) {
        w = static_cast<unsigned int>(wi);
    }
    constexpr void set(Horz hz) {
        h = static_cast<unsigned int>(hz);
    }
    constexpr void setDstHz(Horz hz) {
        set(Vert::VT_INVALID, Width::WI_INVALID, hz);
    }

    Horz    getHz() const {return static_cast<Horz>(h);}
    Vert    getVt() const {return static_cast<Vert>(v);}
    Width   getWi() const {return static_cast<Width>(w);}

    // checks if a region is invalid (assembler will program it to correct bits)
    bool isInvalid() const {
        return *this == INVALID;
    }
    // checks if a region is of the for <V;W,H>
    bool isVWH() const {
        return
            getVt() != Vert::VT_INVALID &&
            getWi() != Width::WI_INVALID &&
            getHz() != Horz::HZ_INVALID;
    }

    // define [in]equality based on bits
    bool operator ==(const Region &b) const {
        // careful: fails if padding differs (we prevent that now)
        return b.bits == bits;
    }
    bool operator !=(const Region &b) const {
        return  !(*this == b);
    }


    // some useful region constants
    static const Region INVALID; // all RESERVED elements
    //
    // dst
    static const Region DST1;    // <1>
    static const Region DST2;    // <2>
    static const Region DST4;    // <4>
    //
    // generalized src regions
    static const Region SRC010;  // <0;1,0> (broadcast scalar)
    static const Region SRC110;  // <1;1,0> (packed access)
    static const Region SRC210;  // <2;1,0> (even strided access)
    static const Region SRC410;  // <4;1,0> (quarter stided access)
    //
    // older src regions
    static const Region SRC221;  // <2;2,1>
    static const Region SRC441;  // <4;4,1>
    static const Region SRC881;  // <8;8,1>
    static const Region SRCFF1;  // <16;16,1>
    //
    // special cases
    // ternary src0/src1
    static const Region SRC0X0;  // <0;0> (ternary align1 src0 and src1)
    static const Region SRC2X1;  // <2;1> (ternary align1 src0 and src1)
    static const Region SRC1X0;  // <1;0> XE_LP changes 2 to 1 in encoding
    static const Region SRC4X1;  // <4;1> (ternary align1 src0 and src1)
    // ternary src2
    static const Region SRC8X1;  // <8;1> (ternary align1 src0 and src1)
    static const Region SRCXX0;  // <0>   (ternary align1 src2)
    static const Region SRCXX1;  // <1>   (ternary align1 src2)
    static const Region SRCXX2;  // <2>   (ternary align1 src2)
};

// A set of instruction options
typedef EnumBitset<InstOpt> InstOptSet;


struct RegRef {
    uint16_t  regNum    = 0;
    uint16_t  subRegNum = 0;

    constexpr RegRef() { }
    constexpr RegRef(uint16_t rNum, uint16_t srNum)
        : regNum(rNum), subRegNum(srNum) { }
    constexpr RegRef(int rNum, int srNum)
        : regNum((uint16_t)rNum), subRegNum((uint16_t)srNum) { }
    constexpr RegRef(uint32_t rNum, uint32_t srNum)
        : regNum((uint16_t)rNum), subRegNum((uint16_t)srNum) { }

    bool operator==(const RegRef &rr) const {
        return regNum == rr.regNum && subRegNum == rr.subRegNum;
    }
    bool operator!=(const RegRef &rr) const {
        return !(*this == rr);
    }
};

static constexpr RegRef REGREF_INVALID {0xFFFF, 0xFFFF};
static constexpr RegRef REGREF_ZERO_ZERO {0, 0};

struct SendDesc {
    enum class Kind {IMM, REG32A};
    Kind type;

    union {
        RegRef         reg;
        uint32_t       imm;
    };

    constexpr SendDesc() : SendDesc(0) { }
    constexpr SendDesc(uint32_t desc) : type(Kind::IMM), imm(desc) { }
    constexpr SendDesc(RegRef a0r) : type(Kind::REG32A), reg(a0r) { }

    bool isReg() const {return type == Kind::REG32A;}
    bool isImm() const {return type == Kind::IMM;}
};

} // namespace
#endif
