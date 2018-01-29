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


namespace iga
{
    enum class RegName
    {
        INVALID = 0, //        ENCODING
        ARF_NULL,  // null      0000b
        ARF_A,     // a#        0001b
        ARF_ACC,   // acc# ...  0010b
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
        NO_MESS    = -4, // no message types on the given SFID
        NON_IMM    = -3, // non-immediate
        ERR        = -2, // error the parameters
        INVALID    = -1,
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

    enum class SFID
    {
        ERR     = -3,
        NON_IMM = -2,
        INVALID = -1,
        NULL_   =  0,  // the null shared function
        SMPL    =  2,  // sampler; Note: Pre-SKL sampler also maps here as well
        GTWY    =  3,  // gateway
        DC2     =  4,  // data cache 2 (HDC)
        RC      =  5,  // render cache (HDC)
        URB     =  6,  // URB (HDC)
        TS      =  7,  // thread spawner
        VME     =  8,  // VME FF's (sampler)
        DCRO    =  9,  // data cache read only (HDC) ...
        DC0     =  10, // data cache 2 (HDC)
        PIXI    =  11, // pixel interpolater (sampler)
        DC1     =  12, // data cache 1 (HDC)
        CRE     =  13, // check and refinement engine
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
}
#endif
