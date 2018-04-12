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
#ifndef IGA_IR_TYPES_HPP
#define IGA_IR_TYPES_HPP

// WARNING: the IR is subject to change without any notice.  External tools
// should use the official interfaces in the external API.  Those interfaces
// are tested between releases and maintained even with changes to the IR.


#include <cstdint>

#include "../EnumBitset.hpp"
#include "../Models/bxml/iga_bxml_enums.hpp"
#include "../api/iga_types_ext.hpp"

namespace iga
{

// The GEN platform version
enum class Platform
{
#define IGA_GEN_VER_ORDINAL(MAJ,MIN) (((MAJ)<<16)|(MIN))
    INVALID     = 0,

    GEN6        = IGA_GEN_VER_ORDINAL( 6, 0),
    GEN7        = IGA_GEN_VER_ORDINAL( 7, 0),
    GEN7P5      = IGA_GEN_VER_ORDINAL( 7, 5),
    GEN8        = IGA_GEN_VER_ORDINAL( 8, 0),
    GEN8LP      = IGA_GEN_VER_ORDINAL( 8, 1),
    GEN9        = IGA_GEN_VER_ORDINAL( 9, 0),
    GEN9LP      = IGA_GEN_VER_ORDINAL( 9, 1),
    GEN9P5      = IGA_GEN_VER_ORDINAL( 9, 5),
    GEN10       = IGA_GEN_VER_ORDINAL(10, 0),
    GENNEXT     = IGA_GEN_VER_ORDINAL(10, 0)

#undef IGA_GEN_VER_ORDINAL
};

enum class PredCtrl
{
    NONE, // predication is off
    SEQ,  // no explicit function; e.g. f0.0
    ANYV, // .anyv; e.g. "f0.0.anyv"
    ALLV,
    ANY2H,
    ALL2H,
    ANY4H,
    ALL4H,
    ANY8H,
    ALL8H,
    ANY16H,
    ALL16H,
    ANY32H,
    ALL32H
};


struct Predication
{
    PredCtrl  function;
    bool      inverse; // TODO: enum

    Predication() : function(PredCtrl::NONE), inverse(false) { }
    Predication(PredCtrl ctnrl, bool inv) : function(ctnrl), inverse(inv) { }
};


enum class BranchCntrl
{
    OFF,
    ON,
};

static inline int ExecSizeToInt(ExecSize es)
{
    return static_cast<int>(es);
}
static inline ExecSize ExecSizeFromInt(int es)
{
    return static_cast<ExecSize>(es);
}


enum class SrcModifier
{
    NONE,
    ABS,
    NEG,
    NEG_ABS,
};


enum class DstModifier
{
    NONE,
    SAT,
};

// for implicit accumulator access (madm, math.invm, and math.rsqrtm)
enum class ImplAcc
{
    INVALID,
    ACC2,  // encodes as 0000b
    ACC3,  // encodes as 0001b
    ACC4,  // encodes as 0010b
    ACC5,  // encodes as 0011b
    ACC6,  // encodes as 0100b
    ACC7,  // encodes as 0101b
    ACC8,  // encodes as 0110b
    ACC9,  // encodes as 0111b
    NOACC, // encodes as 1000b
};


static inline int LogTypeSize(Type type, int dft = -1)
{
    switch (type)
    {
    case Type::DF:
    case Type::Q:
    case Type::UQ:
        return 3;
    case Type::UD:
    case Type::D:
    case Type::F:
    case Type::NF: // Darrin said the :nf accumulators region the same as :f
    case Type::V:
    case Type::VF:
    case Type::UV:
        return 2;
    case Type::UW:
    case Type::W:
    case Type::HF:
        return 1;
    case Type::UB:
    case Type::B:
        return 0;
    default:
        if (dft < 0) {
            IGA_ASSERT_FALSE("TypeSize() on INVALID type");
        }
        return dft;
    }
}
static inline int TypeSize(Type type)
{
    return 1 << LogTypeSize(type);
}
static inline int TypeSizeWithDefault(Type type, int dft = 0)
{
    return type == Type::INVALID ? dft : TypeSize(type);
}

enum class FlagModifier
{
    NONE = 0,  // no flag modification
    EQ,        // equal (zero)
    NE,        // not-equal (not-zer)
    GT,        // greater than
    GE,        // greater than or equal
    LT,        // less than
    LE,        // less than or equal
               // 7 is reserved
    OV = 8,    // overflow
    UN,        // unordered (NaN)
    EO = 0xFF, // early out: special implicit flag modifier for math.invm and math.rsqrtm
};


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

    void set(Vert v, Width w, Horz h) {
        this->bits = 0; // clear padding
        this->v = static_cast<unsigned int>(v);
        this->w =  static_cast<unsigned int>(w);
        this->h =  static_cast<unsigned int>(h);
    }

    void set(Vert vt) {
        v = static_cast<unsigned int>(vt);
    }
    void set(Width wi) {
        w = static_cast<unsigned int>(wi);
    }
    void set(Horz hz) {
        h = static_cast<unsigned int>(hz);
    }
    void setDstHz(Horz hz) {
        set(Vert::VT_INVALID, Width::WI_INVALID, hz);
    }

    Horz    getHz() const { return static_cast<Horz>(h); }
    Vert    getVt() const { return static_cast<Vert>(v); }
    Width   getWi() const { return static_cast<Width>(w); }

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
    static const Region DST1;    // <1>
    static const Region SRC010;  // <0;1,0> (broadcast scalar)
    static const Region SRC110;  // <1;1,0> (packed access)
    static const Region SRC221;  // <2;2,1>
    static const Region SRC441;  // <4;4,1>
    static const Region SRC881;  // <8;8,1>
    static const Region SRCFF1;  // <16;16,1>
    static const Region SRC0X0;  // <0;0> (ternary align1 src0 and src1)
    static const Region SRC2X1;  // <2;1> (ternary align1 src0 and src1)
    static const Region SRC4X1;  // <4;1> (ternary align1 src0 and src1)
    static const Region SRC8X1;  // <8;1> (ternary align1 src0 and src1)
    static const Region SRCXX0;  // <0>   (ternary align1 src2)
    static const Region SRCXX1;  // <1>   (ternary align1 src2)
};


// instruction options
enum class InstOpt {
    ACCWREN,
    ATOMIC,
    BREAKPOINT,
    COMPACTED,
    EOT,
    NOCOMPACT,
    NODDCHK,
    NODDCLR,
    NOPREEMPT,
    NOSRCDEPSET,
    SWITCH
};

// A set of instruction options
typedef EnumBitset<InstOpt> InstOptSet;


struct RegRef {
    uint8_t  regNum;
    uint8_t  subRegNum;

    bool operator==(const RegRef &rr) const {
        return regNum == rr.regNum && subRegNum == rr.subRegNum;
    }
    bool operator!=(const RegRef &rr) const {
        return !(*this == rr);
    }

};

static const RegRef REGREF_INVALID = {0xFF,0xFF};
static const RegRef REGREF_ZERO_ZERO = {0,0};

struct SendDescArg {
    enum {IMM, REG32A}  type;
    union {
        RegRef         reg;
        uint32_t       imm;
    };
};
} // namespace
#endif
