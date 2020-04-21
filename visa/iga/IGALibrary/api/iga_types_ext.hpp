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
#ifndef _IGA_TYPES_EXT_HPP
#define _IGA_TYPES_EXT_HPP

#include "iga_bxml_ops.hpp"
#include "iga_bxml_enums.hpp"
#include "iga_types_swsb.hpp"

namespace iga
{
    enum class RegName
    {
        INVALID = 0, //        ENCODING
        ARF_NULL,  // null      0000b
        ARF_A,     // a#        0001b
        ARF_ACC,   // acc# ...  0010b with RegNum[3:0] = 0..1 for acc0-acc1
                   // GEN12                              0..3 for acc0-acc3

        // Math Macro Exponent (or Extended).
        //
        // Holds a 2b part of the exponent for IEEE-accurate math sequences
        // using madm and math.{invm,rsqrtm}
        // Formerly known as acc2-acc9.  Still encodes the same.
        // sometimes called "Special Accumulators"
        // formerly called acc2-acc9 used in madm and
        ARF_MME,   // mme# ...  0010b with RegNum[3:0] = 2..9  for mme0-mme7
                   //           GEN12      RegNum[3:0] = 8..15 for mme0-mme7

        ARF_F,     // f#        0011b
        ARF_CE,    // ce        0100b
        ARF_MSG,   // msg#      0101b
        ARF_SP,    // sp        0110b
        ARF_SR,    // sr#       0111b
        ARF_CR,    // cr#       1000b
        ARF_N,     // n#        1001b
        ARF_IP,    // ip        1010b
        ARF_TDR,   // tdr       1011b
        ARF_TM,    // tm#       1100b
        ARF_FC,    // fc#       1101b
        //                       ... reserved
        ARF_DBG,   // dbg0      1111b
        GRF_R,     // GRF       N/A
    };

    enum class Type
    {
        INVALID,


        UB,
        B,
        UW,
        W,
        UD,
        D,
        UQ,
        Q,

        HF,
        F,
        DF,
        NF,

        V,
        UV,
        VF,
    };

    // an operand type
    enum class Kind {
        INVALID,   // an invalid or uninitialized operand
        DIRECT,    // direct register reference
        MACRO,     // madm or math.invm or math.rsqrtm
        INDIRECT,  // register-indriect access
        IMMEDIATE, // immediate value
        LABEL,     // block target (can be numeric label/i.e. imm value)
    };

    enum class SFMessageType
    {
        INVALID = -1,

        MSD0R_HWB,
        MSD0W_HWB,
        MT0R_OWB,
        MT0R_OWUB,
        MT0R_OWDB,
        MT0R_DWS,
        MT0R_BS,
        MT0_MEMFENCE,
        MT0W_OWB,
        MT0W_OWDB,
        MT0W_DWS,
        MT0W_BS,
        MT1R_T,
        MT1R_US,
        MT1A_UI,
        MT1A_UI4x2,
        MT1R_MB,
        MT1R_TS,
        MT1A_TA,
        MT1A_TA4x2,
        MT1W_US,
        MT1W_MB,
        MT1A_TC,
        MT1A_TC4x2,
        MT1W_TS,
        MT1R_A64_SB,
        MT1R_A64_US,
        MT1A_A64_UI,
        MT1A_A64_UI4x2,
        MT1R_A64_B,
        MT1W_A64_B,
        MT1W_A64_US,
        MT1W_A64_SB,
        MT2R_US,
        MT2R_A64_SB,
        MT2R_A64_US,
        MT2R_BS,
        MT2W_US,
        MT2W_A64_US,
        MT2W_A64_SB,
        MT2W_BS,
        MT_CC_OWB,
        MT_CC_OWUB,
        MT_CC_OWDB,
        MT_CC_DWS,
        MT_SC_OWUB,
        MT_SC_MB,
        MT_RSI,
        MT_RTW,
        MT_RTR,
        MTR_MB,
        MTRR_TS,
        MTRA_TA,
        MT_MEMFENCE,
        MTW_MB,
        MTRW_TS,
        MT0R_US,
        MT0A_UI,
        MT0W_US,
        MT1A_UF4x2,
        MT1A_UF,
        MT1A_A64_UF,
        MT1A_A64_UF4x2,
    };

    enum class ExecSize
    {
        INVALID = 0,

        SIMD1 = 1,
        SIMD2 = 2,
        SIMD4 = 4,
        SIMD8 = 8,
        SIMD16 = 16,
        SIMD32 = 32,
    };

    enum class ChannelOffset
    {
        M0,
        M4,
        M8,
        M12,
        M16,
        M20,
        M24,
        M28,
    };

    enum class MaskCtrl
    {
        NORMAL,
        NOMASK,
    };

    enum class FlagModifier
    {
        NONE = 0,  // no flag modification
        EQ,        // equal (zero)
        NE,        // not-equal (not-zer)
        GT,        // greater than
        GE,        // greater than or equal
        LT,        // less than
        LE,        // less than or equal
                   // Reserved <= 7
        OV = 8,    // overflow
        UN,        // unordered (NaN)
        EO = 0xFF, // special implicit flag modifier for math.invm and math.rsqrtm
                   // [trb] from Balakumar Rajendran (Apr 2016)
                   // "The .eo means early out.  It doesn't have bits in the
                   // instruction when a macro is executed.  The flag is set
                   // for early out conditions including division by
                   // 0, infinity, etc"
    };

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

    enum class PredCtrl
    {
        NONE, // predication is off
        SEQ,  // no explicit function; e.g. f0.0
        ANYV, // .anyv; e.g. "f0.0.anyv". any from f0.0-f1.0 on the same channel
        ALLV, // all from f0.0-f1.0 on the same channel
        ANY2H,
        ALL2H,
        ANY4H,
        ALL4H,
        ANY8H,
        ALL8H,
        ANY16H,
        ALL16H,
        ANY32H,
        ALL32H,
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
        SWITCH,
        SERIALIZE,
    };
} // namespace iga
#endif
