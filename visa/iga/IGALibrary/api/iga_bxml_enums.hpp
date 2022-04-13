/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_BXML_TYPES_HPP
#define IGA_BXML_TYPES_HPP

// C++17 allows inline variables, but until then we need a data structure
#include <array>
#include <cstdint>

namespace iga
{
    struct BfnFC {
        uint32_t value;
        constexpr explicit BfnFC(uint8_t val) : value(val) { }
        const char *c_str() const;
    };

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
        UGML, // untyped global memory (low bandwidth) for XeHPC
        SMPL, // sampler
        GTWY, // gateway
        DC2,  // data cache 2
        RC,   // render cache
        URB,  // unified return buffer
        TS,   // thread spawner (until XeHPG)
        VME,  // video motion estimation (until XeHPG)
        DCRO, // data cache read-only
        DC0,  // data cache 0
        PIXI, // pixel interpolator
        DC1,  // data cache 1
        CRE,  // check and refinement engine (until XeHPG)
        SLM, // shared local memory (XeHPG+)
        UGM, // untyped global memory (XeHPG+)
        BTD, // bindless thread dispatcher (XeHPG+)
        RTA, // ray tracing accellerator (XeHPG+)
        TGM, // typed global memory (XeHPG+)
        A0REG  = 0x100, // for <XE SFID can be indirect
    };
    static const std::array<SFID,
        19
    > ALL_SFIDS {
        SFID::NULL_,
        SFID::UGML,
        SFID::SMPL, SFID::GTWY, SFID::DC2, SFID::RC, SFID::URB,
        SFID::TS, SFID::VME, SFID::DCRO, SFID::DC0, SFID::PIXI,
        SFID::DC1, SFID::CRE,
        SFID::SLM, SFID::UGM, SFID::BTD, SFID::RTA, SFID::TGM,
    };


    // The sync instruction's subfunction
    enum class SyncFC
    {
        INVALID = -1,
        NOP    =   0,
        ALLRD  =   2,
        ALLWR  =   3,
        FENCE  =  13, // XeHPC
        BAR    =  14,
        HOST   =  15,
    };
    // static const SyncFC ALL_SyncFCs[] ...
    static const std::array<SyncFC,
        6
        > ALL_SyncFCs {
        SyncFC::NOP,
        SyncFC::ALLRD,
        SyncFC::ALLWR,
        SyncFC::FENCE, // XeHPC
        SyncFC::BAR,
        SyncFC::HOST,
    };

    // The DpasFC function control fuses the systolic count and repeat counts
    // into a single subfunction.
    enum class DpasFC
    {
        // ordinal values are now (SystolicCount << 8) | (RepeatCount)
        // encoding has to do something special because it's platform specific
        INVALID = -1,
        //
        F_1X1  =  (1 << 8) | 1,
        F_1X2  =  (1 << 8) | 2,
        F_1X3  =  (1 << 8) | 3,
        F_1X4  =  (1 << 8) | 4,
        F_1X5  =  (1 << 8) | 5,
        F_1X6  =  (1 << 8) | 6,
        F_1X7  =  (1 << 8) | 7,
        F_1X8  =  (1 << 8) | 8,
        F_2X1  =  (2 << 8) | 1,
        F_2X2  =  (2 << 8) | 2,
        F_2X3  =  (2 << 8) | 3,
        F_2X4  =  (2 << 8) | 4,
        F_2X5  =  (2 << 8) | 5,
        F_2X6  =  (2 << 8) | 6,
        F_2X7  =  (2 << 8) | 7,
        F_2X8  =  (2 << 8) | 8,
        F_4X1  =  (4 << 8) | 1,
        F_4X2  =  (4 << 8) | 2,
        F_4X3  =  (4 << 8) | 3,
        F_4X4  =  (4 << 8) | 4,
        F_4X5  =  (4 << 8) | 5,
        F_4X6  =  (4 << 8) | 6,
        F_4X7  =  (4 << 8) | 7,
        F_4X8  =  (4 << 8) | 8,
        F_8X1  =  (8 << 8) | 1,
        F_8X2  =  (8 << 8) | 2,
        F_8X3  =  (8 << 8) | 3,
        F_8X4  =  (8 << 8) | 4,
        F_8X5  =  (8 << 8) | 5,
        F_8X6  =  (8 << 8) | 6,
        F_8X7  =  (8 << 8) | 7,
        F_8X8  =  (8 << 8) | 8,
    };

    // static const DpasFC ALL_DpasFCs[] ...
    static const std::array<DpasFC,48> ALL_DpasFCs {
        DpasFC::F_1X1, DpasFC::F_1X2, DpasFC::F_1X3,
        DpasFC::F_1X4, DpasFC::F_1X5, DpasFC::F_1X6,
        DpasFC::F_1X7, DpasFC::F_1X8,
        DpasFC::F_2X1, DpasFC::F_2X2, DpasFC::F_2X3,
        DpasFC::F_2X4, DpasFC::F_2X5, DpasFC::F_2X6,
        DpasFC::F_2X7, DpasFC::F_2X8,
        DpasFC::F_4X1, DpasFC::F_4X2, DpasFC::F_4X3,
        DpasFC::F_4X4, DpasFC::F_4X5, DpasFC::F_4X6,
        DpasFC::F_4X7, DpasFC::F_4X8,
        DpasFC::F_8X1, DpasFC::F_8X2, DpasFC::F_8X3,
        DpasFC::F_8X4, DpasFC::F_8X5, DpasFC::F_8X6,
        DpasFC::F_8X7, DpasFC::F_8X8,
    };

    static inline uint32_t GetDpasSystolicDepth(DpasFC sf) {
        return uint32_t(sf) >> 8;
    }
    static inline uint32_t GetDpasRepeatCount(DpasFC sf) {
        return uint32_t(sf) & 0xFF;
    }
    static inline uint32_t GetDpasSystolicDepthEncoding(
        DpasFC sf, uint32_t divisor)
    {
        switch (GetDpasSystolicDepth(sf)/divisor) {
        case 1: return 0;
        case 2: return 1;
        case 4: return 2;
        case 8: return 3;
        default: return 0xFFFFFFFF; // unreachable
        }
    }
    static inline uint32_t GetDpasRepeatCountEncoding(DpasFC sf) {
        return GetDpasRepeatCount(sf) - 1;
    }
    static inline DpasFC GetDpasFC(uint32_t sysD, uint32_t repC) {
        return DpasFC((sysD << 8) | repC);
    }

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
    // E.g. DPAS has both a systolic depth and repeat count.
    // One way to pair that is to use the high half of the integer
    // to hold one, and the low half to hold the other.
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
            BfnFC       bfn; // BFN subfunction index (low 8 bits are relevent)
            DpasFC      dpas; // op == Op::DPAS || op == Op::DPASW
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
        constexpr Subfunction(BfnFC bf) : bfn(bf) { }
        constexpr Subfunction(DpasFC sf) : dpas(sf) { }

        bool isValid() const {return invalid != InvalidFC::INVALID;}
    };
    static_assert(sizeof(Subfunction) == 4, "Subfunction should be 4B");
}
#endif // _IGA_BXML_TYPES_HPP
