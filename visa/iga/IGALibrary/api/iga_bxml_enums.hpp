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
#ifndef IGA_BXML_TYPES_HPP
#define IGA_BXML_TYPES_HPP

// C++17 allows inline variables, but until then we need a data structure
#include <array>
#include <cstdint>

namespace iga
{

    // used for if, else, and goto
    enum class BranchCtrl
    {
        OFF,
        ON,
    };

    // used for instructions that lack a subfunction
    enum class InvalidFC
    {
        INVALID = -1
    };


    // Math function control (selector)
    enum class MathFC
    {
        INVALID = -1,
        INV = 1,
        LOG,
        EXP,
        SQT,
        RSQT,
        SIN,
        COS,
        FDIV,
        POW,
        IDIV,
        IQOT,
        IREM,
        INVM,
        RSQTM,
    };
    // static const MathFC ALL_MathFCs[] ... need C++17
    static const std::array<MathFC,
        14
    > ALL_MathFCs {
        MathFC::INV, MathFC::LOG, MathFC::EXP, MathFC::SQT, MathFC::RSQT,
        MathFC::SIN, MathFC::COS, MathFC::FDIV, MathFC::POW,
        MathFC::IDIV, MathFC::IQOT, MathFC::IREM, MathFC::INVM, MathFC::RSQTM,
    };
    unsigned GetSourceCount(MathFC mfc);
    static inline bool IsMacro(MathFC mfc) {
        return mfc == MathFC::INVM || mfc == MathFC::RSQTM;
    }

    // Send shared function ID
    enum class SFID
    {
        INVALID = -1,
        NULL_ = 0, // the null shared function
        SMPL, // sampler
        GTWY, // gateway
        DC2,  // data cache 2
        RC,   // render cache
        URB,  // unified return buffer
        TS,   // thread spawner
        VME,  // video motion estimation
        DCRO, // data cache read-only
        DC0,  // data cache 0
        PIXI, // pixel interpolator
        DC1,  // data cache 1
        CRE,  // check and refinement engine
        A0REG  = 0x100, // for <GEN12 SFID can be indirect
    };
    // static const SFID ALL_SFIDS[]
    static const std::array<SFID,
        13
    > ALL_SFIDS {
        SFID::NULL_,
        SFID::SMPL, SFID::GTWY, SFID::DC2, SFID::RC, SFID::URB,
        SFID::TS, SFID::VME, SFID::DCRO, SFID::DC0, SFID::PIXI,
        SFID::DC1, SFID::CRE,
    };



    // The sync instruction's subfunction
    enum class SyncFC
    {
        INVALID = -1,
        NOP    =   0,
        ALLRD  =   2,
        ALLWR  =   3,
        BAR    =  14,
        HOST   =  15,
    };
    // static const SyncFC ALL_SyncFCs[] ...
    static const std::array<SyncFC,
        5
        > ALL_SyncFCs {
        SyncFC::NOP,
        SyncFC::ALLRD,
        SyncFC::ALLWR,
        SyncFC::BAR,
        SyncFC::HOST,
    };




    // An instruction subfunction is a set of immediate-encoded control bits
    // that control the behavior of some EU instruction.  We permit an
    // Instruction to have exactly one of these fields.
    //
    // For instance, the 'math' instruction (Op::MATH) must have a subfunction
    // to select the specific math operation to execute.
    //
    //
    // This is a union data type containing all the valid subfunctions
    // that an instruction may have
    // e.g.
    //    send.<SUBFUNC> ...
    //    sync.<SUBFUNC> ...
    //    math.<SUBFUNC> ...
    // For instructions that contain multiple paramters pair the two
    // together into a single value via a pairing function
    // (search for "Pairing Function" if unfamiliar).
    //
    // NOTE: it's fine to use struct pair{uint16_t x; uint16_t y;};
    // so long as the structure fits in 4B (we could make this wider, but
    // don't go crazy and beware of issues with ragged union sizes).
    //
    // **: SFID is a littly wonky here since in <=GEN11 it is part of the
    // send extended descriptor.  Hence, parsing and decoding have to get
    // it in different parts of their respective algorithms.
    struct Subfunction {
        union {
            InvalidFC   invalid; // for operations without a subfunction
            BranchCtrl  branch; // for if/else/goto on GEN8+
            MathFC      math; // op == Op::MATH
            SFID        send; // op == Op::SEND*  (**)
            SyncFC      sync; // op == Op::SYNC
            // NOTE: this does *not* correspond to any direct encoding
            // since that can vary from platform to platform.
            uint32_t    bits;
        };
        constexpr Subfunction() : Subfunction(InvalidFC::INVALID) { }
        constexpr Subfunction(InvalidFC sf) : invalid(sf) { }
        constexpr Subfunction(BranchCtrl sf) : branch(sf) { }
        constexpr Subfunction(MathFC sf) : math(sf) { }
        constexpr Subfunction(SFID sf) : send(sf) { }
        constexpr Subfunction(SyncFC sf) : sync(sf) { }

        bool isValid() const {return invalid != InvalidFC::INVALID;}
    };
    static_assert(sizeof(Subfunction) == 4, "Subfunction should be 4B");
}
#endif // _IGA_BXML_TYPES_HPP
