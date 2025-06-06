/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SystemThread.h"
#include "common/allocator.h"
#include "common/StateSaveAreaHeader.h"
#include "common/igc_regkeys.hpp"
#include "common/secure_mem.h"
#include "common/debug/Dump.hpp"

#include "common/SIPKernels/Gen10SIPCSR.h"
#include "common/SIPKernels/Gen10SIPCSRDebug.h"
#include "common/SIPKernels/Gen10SIPDebug.h"
#include "common/SIPKernels/Gen10SIPDebugBindless.h"
#include "common/SIPKernels/Gen9BXTSIPCSR.h"
#include "common/SIPKernels/Gen9SIPCSR.h"
#include "common/SIPKernels/Gen9SIPCSRDebug.h"
#include "common/SIPKernels/Gen9SIPCSRDebugLocal.h"
#include "common/SIPKernels/Gen9SIPDebug.h"
#include "common/SIPKernels/Gen9SIPDebugBindless.h"
#include "common/SIPKernels/Gen9GLVSIPCSR.h"
#include "common/SIPKernels/Gen11SIPCSR.h"
#include "common/SIPKernels/Gen11SIPCSRDebug.h"
#include "common/SIPKernels/Gen11SIPCSRDebugBindless.h"
#include "common/SIPKernels/Gen11LKFSIPCSR.h"
#include "common/SIPKernels/Gen12LPSIPCSR.h"
#include "common/SIPKernels/Gen12LPSIPCSRDebug.h"
#include "common/SIPKernels/Gen12LPSIPCSRDebugBindless.h"
#include "Probe/Assertion.h"
#include "common/SIPKernels/XeHPSIPCSR.h"
#include "common/SIPKernels/XeHPSIPCSRDebug.h"
#include "common/SIPKernels/XeHPSIPCSRDebugBindless.h"
#include "common/SIPKernels/XeHPGSIPCSRDebug.h"
#include "common/SIPKernels/XeHPGSIPCSRDebugBindless.h"
#include "common/SIPKernels/XeHPCSIPCSRDebugBindless.h"
#include "common/SIPKernels/Xe2SIPCSRDebugBindless.h"
#include "common/SIPKernels/Xe3_G_SIPDebugBindless.h"
#include "common/SIPKernels/wmtp/Xe3_PTL_config_1x4.h"
#include "common/SIPKernels/wmtp/Xe3_PTL_config_2x6.h"
#include "common/SIPKernels/wmtp/XE2_config_128.h"
#include "common/SIPKernels/wmtp/XE2_config_160.h"


using namespace llvm;
using namespace USC;

namespace SIP
{
// wmtp PTL SIP
struct StateSaveAreaHeaderV4 Xe3SIP_config_1x4_WMTP_CSRDebugBindlessDebugHeader =
{
    {"tssarea", 0, {4, 0, 0}, sizeof(StateSaveAreaHeaderV4) / 8, {0, 0, 0}}, // versionHeader
    XE3_CSR_DEBUG_BINDLESS_PTL_config_1x4_WMTP_DATA_SIZE // total_wmtp_data_size
};

struct StateSaveAreaHeaderV4 Xe3SIP_config_2x6_WMTP_CSRDebugBindlessDebugHeader =
{
    {"tssarea", 0, {4, 0, 0}, sizeof(StateSaveAreaHeaderV4) / 8, {0, 0, 0}}, // versionHeader
    XE3_CSR_DEBUG_BINDLESS_PTL_config_2x6_WMTP_DATA_SIZE // total_wmtp_data_size
};

struct Xe3_G_DebugSurfaceLayout
{
    // The *_ALIGN fields below are padding of the SIP between
    // the registers set.
    static constexpr size_t GR_COUNT = 256;
    static constexpr size_t GR_ELEMENTS = 1;
    static constexpr size_t GR_ELEMENT_SIZE = 64;
    static constexpr size_t GR_ALIGN = 0;

    static constexpr size_t A0_COUNT = 1;
    static constexpr size_t A0_ELEMENTS = 16;
    static constexpr size_t A0_ELEMENT_SIZE = 2;
    static constexpr size_t A0_ALIGN = 0x20;

    static constexpr size_t F_COUNT = 4;
    static constexpr size_t F_ELEMENTS = 2;
    static constexpr size_t F_ELEMENT_SIZE = 2;
    static constexpr size_t F_ALIGN = 0;

    static constexpr size_t EXEC_MASK_COUNT = 1;
    static constexpr size_t EXEC_MASK_ELEMENTS = 1;
    static constexpr size_t EXEC_MASK_ELEMENT_SIZE = 4;
    static constexpr size_t EXEC_MASK_ALIGN = 0;

    static constexpr size_t SR_COUNT = 1;
    static constexpr size_t SR_ELEMENTS = 5;
    static constexpr size_t SR_ELEMENT_SIZE = 4;
    static constexpr size_t SR_ALIGN = 0;

    static constexpr size_t CR_COUNT = 1;
    static constexpr size_t CR_ELEMENTS = 3;
    static constexpr size_t CR_ELEMENT_SIZE = 4;
    static constexpr size_t CR_ALIGN = 4;

    static constexpr size_t N_COUNT = 1;
    static constexpr size_t N_ELEMENTS = 3;
    static constexpr size_t N_ELEMENT_SIZE = 4;
    static constexpr size_t N_ALIGN = 0;

    static constexpr size_t TDR_COUNT = 0; // TDR not valid for XE3
    static constexpr size_t TDR_ELEMENTS = 0;
    static constexpr size_t TDR_ELEMENT_SIZE = 0;
    static constexpr size_t TDR_ALIGN = 0;

    static constexpr size_t ACC_COUNT = 4;
    static constexpr size_t ACC_ELEMENTS = 16;
    static constexpr size_t ACC_ELEMENT_SIZE = 4; // actually defined as 33 bits, verify how these should be stored
    static constexpr size_t ACC_ALIGN = 0xC0;

    // MSG is used to access MME msg0[0-2]:ud, msg1[0-7]:ud
    // define as 2 8 element registers
    static constexpr size_t MSG_COUNT = 2;
    static constexpr size_t MSG_ELEMENTS = 8;
    static constexpr size_t MSG_ELEMENT_SIZE = 4;
    static constexpr size_t MSG_ALIGN = 16;

    static constexpr size_t MME_COUNT = 8;
    static constexpr size_t MME_ELEMENTS = 1;
    static constexpr size_t MME_ELEMENT_SIZE = 4;
    static constexpr size_t MME_ALIGN = 0;

    static constexpr size_t TM_COUNT = 1;
    static constexpr size_t TM_ELEMENTS = 5;
    static constexpr size_t TM_ELEMENT_SIZE = 4;
    static constexpr size_t TM_ALIGN = 8;

    static constexpr size_t CE_COUNT = 1;
    static constexpr size_t CE_ELEMENTS = 1;
    static constexpr size_t CE_ELEMENT_SIZE = 4;
    static constexpr size_t CE_ALIGN = 0;

    static constexpr size_t DBG_COUNT = 1;
    static constexpr size_t DBG_ELEMENTS = 1;
    static constexpr size_t DBG_ELEMENT_SIZE = 4;
    static constexpr size_t DBG_ALIGN = 4;

    static constexpr size_t VERSION_COUNT = 1;
    static constexpr size_t VERSION_ELEMENTS = 1;
    static constexpr size_t VERSION_ELEMENT_SIZE = 20; // sizeof(sr_ident);
    static constexpr size_t VERSION_ALIGN = 44;        // aligning scalar/cmd

    static constexpr size_t SIP_CMD_COUNT = 1;
    static constexpr size_t SIP_CMD_ELEMENTS = 1;
    static constexpr size_t SIP_CMD_ELEMENT_SIZE = 128;
    static constexpr size_t SIP_CMD_ALIGN = 0;

    static constexpr size_t CONTEXT_ID_COUNT = 1;
    static constexpr size_t CONTEXT_ID_ELEMENTS = 1;
    static constexpr size_t CONTEXT_ID_ELEMENT_SIZE = 8;
    static constexpr size_t CONTEXT_ID_ALIGN = 8;

    static constexpr size_t DBG_REG_COUNT = 1;
    static constexpr size_t DBG_REG_ELEMENTS = 3;
    static constexpr size_t DBG_REG_ELEMENT_SIZE = 4;
    static constexpr size_t DBG_REG_ALIGN = 8;

    static constexpr size_t FC_COUNT = 3;
    static constexpr size_t FC_ELEMENTS = 16;
    static constexpr size_t FC_ELEMENT_SIZE = 4;
    static constexpr size_t FC_ALIGN = 0;

    static constexpr size_t SCALAR_REG_COUNT = 1;
    static constexpr size_t SCALAR_REG_ELEMENTS = 32;
    static constexpr size_t SCALAR_REG_ELEMENT_SIZE = 1;
    static constexpr size_t SCALAR_REG_ALIGN = 32;

    static constexpr size_t Xe3_G_STATE_SAVE_AREA_SIZE = 0x4500;

    uint8_t grf[GR_COUNT * GR_ELEMENTS * GR_ELEMENT_SIZE + GR_ALIGN];
    uint8_t a0[A0_COUNT * A0_ELEMENTS * A0_ELEMENT_SIZE + A0_ALIGN];
    uint8_t f[F_COUNT * F_ELEMENTS * F_ELEMENT_SIZE + F_ALIGN];
    uint8_t sr[SR_COUNT * SR_ELEMENTS * SR_ELEMENT_SIZE + SR_ALIGN];
    uint8_t cr[CR_COUNT * CR_ELEMENTS * CR_ELEMENT_SIZE + CR_ALIGN];
    uint8_t n[N_COUNT * N_ELEMENTS * N_ELEMENT_SIZE + N_ALIGN];
    uint8_t acc[ACC_COUNT * ACC_ELEMENTS * ACC_ELEMENT_SIZE + ACC_ALIGN];
    uint8_t mme[MME_COUNT * MME_ELEMENTS * MME_ELEMENT_SIZE + MME_ALIGN];
    uint8_t msg[MSG_COUNT * MSG_ELEMENTS * MSG_ELEMENT_SIZE + MSG_ALIGN];
    // uint8_t tdr[TDR_COUNT * TDR_ELEMENTS * TDR_ELEMENT_SIZE + TDR_ALIGN];
    uint8_t fc[FC_COUNT * FC_ELEMENTS * FC_ELEMENT_SIZE + FC_ALIGN];
    uint8_t tm[TM_COUNT * TM_ELEMENTS * TM_ELEMENT_SIZE + TM_ALIGN];
    uint8_t execmask[EXEC_MASK_COUNT * EXEC_MASK_ELEMENTS * EXEC_MASK_ELEMENT_SIZE + EXEC_MASK_ALIGN];
    uint8_t ctx[CONTEXT_ID_COUNT * CONTEXT_ID_ELEMENTS * CONTEXT_ID_ELEMENT_SIZE + CONTEXT_ID_ALIGN];
    uint8_t dbg_reg[DBG_REG_COUNT * DBG_REG_ELEMENTS * DBG_REG_ELEMENT_SIZE + DBG_REG_ALIGN];
    uint8_t ce[CE_COUNT * CE_ELEMENTS * CE_ELEMENT_SIZE + CE_ALIGN];
    uint8_t dbg[DBG_COUNT * DBG_ELEMENTS * DBG_ELEMENT_SIZE + DBG_ALIGN];
    uint8_t version[VERSION_COUNT * VERSION_ELEMENTS * VERSION_ELEMENT_SIZE + VERSION_ALIGN];
    uint8_t s[SCALAR_REG_COUNT * SCALAR_REG_ELEMENTS * SCALAR_REG_ELEMENT_SIZE + SCALAR_REG_ALIGN];
    uint8_t sip_cmd[SIP_CMD_COUNT * SIP_CMD_ELEMENTS * SIP_CMD_ELEMENT_SIZE + SIP_CMD_ALIGN];
};

struct StateSaveAreaHeaderV3 Xe3_G_SIPDebugBindlessDebugHeaderV3 = {
    {"tssarea", 0, {3, 0, 0}, sizeof(StateSaveAreaHeaderV3) / 8, {0, 0, 0}}, // versionHeader
    {
        // regHeader
        0,                                                    // num_slices
        0,                                                    // num_subslices_per_slice
        0,                                                    // num_eus_per_subslice
        0,                                                    // num_threads_per_eu
        0,                                                    // state_area_offset
        Xe3_G_DebugSurfaceLayout::Xe3_G_STATE_SAVE_AREA_SIZE, // state_save_size
        0,                                                    // slm_area_offset
        0,                                                    // slm_bank_size
        0,                                                    // slm_bank_valid
        offsetof(struct Xe3_G_DebugSurfaceLayout, version),   // sr_magic_offset
        0,                                                    // fifo_offset
        0,                                                    // fifo_size,
        0,                                                    // fifo_head
        0,                                                    // fifo_tail
        0,                                                    // fifo_version
        {0},                                                  // reserved1
        0,                                                    // sip_flags
        {offsetof(struct Xe3_G_DebugSurfaceLayout, grf), Xe3_G_DebugSurfaceLayout::GR_COUNT,
         Xe3_G_DebugSurfaceLayout::GR_ELEMENTS *Xe3_G_DebugSurfaceLayout::GR_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::GR_ELEMENTS *Xe3_G_DebugSurfaceLayout::GR_ELEMENT_SIZE}, // grf
        {offsetof(struct Xe3_G_DebugSurfaceLayout, a0), Xe3_G_DebugSurfaceLayout::A0_COUNT,
         Xe3_G_DebugSurfaceLayout::A0_ELEMENTS *Xe3_G_DebugSurfaceLayout::A0_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::A0_ELEMENTS *Xe3_G_DebugSurfaceLayout::A0_ELEMENT_SIZE}, // addr
        {offsetof(struct Xe3_G_DebugSurfaceLayout, f), Xe3_G_DebugSurfaceLayout::F_COUNT,
         Xe3_G_DebugSurfaceLayout::F_ELEMENTS *Xe3_G_DebugSurfaceLayout::F_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::F_ELEMENTS *Xe3_G_DebugSurfaceLayout::F_ELEMENT_SIZE}, // flag
        {offsetof(struct Xe3_G_DebugSurfaceLayout, execmask), Xe3_G_DebugSurfaceLayout::EXEC_MASK_COUNT,
         Xe3_G_DebugSurfaceLayout::EXEC_MASK_ELEMENTS *Xe3_G_DebugSurfaceLayout::EXEC_MASK_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::EXEC_MASK_ELEMENTS *Xe3_G_DebugSurfaceLayout::EXEC_MASK_ELEMENT_SIZE}, // emask
        {offsetof(struct Xe3_G_DebugSurfaceLayout, sr), Xe3_G_DebugSurfaceLayout::SR_COUNT,
         Xe3_G_DebugSurfaceLayout::SR_ELEMENTS *Xe3_G_DebugSurfaceLayout::SR_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::SR_ELEMENTS *Xe3_G_DebugSurfaceLayout::SR_ELEMENT_SIZE}, // sr
        {offsetof(struct Xe3_G_DebugSurfaceLayout, cr), Xe3_G_DebugSurfaceLayout::CR_COUNT,
         Xe3_G_DebugSurfaceLayout::CR_ELEMENTS *Xe3_G_DebugSurfaceLayout::CR_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::CR_ELEMENTS *Xe3_G_DebugSurfaceLayout::CR_ELEMENT_SIZE}, // cr
        {offsetof(struct Xe3_G_DebugSurfaceLayout, n), Xe3_G_DebugSurfaceLayout::N_COUNT,
         Xe3_G_DebugSurfaceLayout::N_ELEMENTS *Xe3_G_DebugSurfaceLayout::N_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::N_ELEMENTS *Xe3_G_DebugSurfaceLayout::N_ELEMENT_SIZE}, // notification
        {0, 0, // offsetof(struct Xe3_G_DebugSurfaceLayout, tdr), Xe3_G_DebugSurfaceLayout::TDR_COUNT,
         Xe3_G_DebugSurfaceLayout::TDR_ELEMENTS *Xe3_G_DebugSurfaceLayout::TDR_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::TDR_ELEMENTS *Xe3_G_DebugSurfaceLayout::TDR_ELEMENT_SIZE}, // tdr
        {offsetof(struct Xe3_G_DebugSurfaceLayout, acc), Xe3_G_DebugSurfaceLayout::ACC_COUNT,
         Xe3_G_DebugSurfaceLayout::ACC_ELEMENTS *Xe3_G_DebugSurfaceLayout::ACC_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::ACC_ELEMENTS *Xe3_G_DebugSurfaceLayout::ACC_ELEMENT_SIZE}, // acc
        {offsetof(struct Xe3_G_DebugSurfaceLayout, mme), Xe3_G_DebugSurfaceLayout::MME_COUNT,
         Xe3_G_DebugSurfaceLayout::MME_ELEMENTS *Xe3_G_DebugSurfaceLayout::MME_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::MME_ELEMENTS *Xe3_G_DebugSurfaceLayout::MME_ELEMENT_SIZE}, // mme
        {offsetof(struct Xe3_G_DebugSurfaceLayout, ce), Xe3_G_DebugSurfaceLayout::CE_COUNT,
         Xe3_G_DebugSurfaceLayout::CE_ELEMENTS *Xe3_G_DebugSurfaceLayout::CE_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::CE_ELEMENTS *Xe3_G_DebugSurfaceLayout::CE_ELEMENT_SIZE}, // ce
        {0, 0, 0, 0},                                                                       // sp
        {offsetof(struct Xe3_G_DebugSurfaceLayout, sip_cmd), Xe3_G_DebugSurfaceLayout::SIP_CMD_COUNT,
         Xe3_G_DebugSurfaceLayout::SIP_CMD_ELEMENTS *Xe3_G_DebugSurfaceLayout::SIP_CMD_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::SIP_CMD_ELEMENTS *Xe3_G_DebugSurfaceLayout::SIP_CMD_ELEMENT_SIZE}, // cmd
        {offsetof(struct Xe3_G_DebugSurfaceLayout, tm), Xe3_G_DebugSurfaceLayout::TM_COUNT,
         Xe3_G_DebugSurfaceLayout::TM_ELEMENTS *Xe3_G_DebugSurfaceLayout::TM_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::TM_ELEMENTS *Xe3_G_DebugSurfaceLayout::TM_ELEMENT_SIZE}, // tm
        {offsetof(struct Xe3_G_DebugSurfaceLayout, fc), Xe3_G_DebugSurfaceLayout::FC_COUNT,
         Xe3_G_DebugSurfaceLayout::FC_ELEMENTS *Xe3_G_DebugSurfaceLayout::FC_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::FC_ELEMENTS *Xe3_G_DebugSurfaceLayout::FC_ELEMENT_SIZE}, // FC
        {offsetof(struct Xe3_G_DebugSurfaceLayout, dbg), Xe3_G_DebugSurfaceLayout::DBG_COUNT,
         Xe3_G_DebugSurfaceLayout::DBG_ELEMENTS *Xe3_G_DebugSurfaceLayout::DBG_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::DBG_ELEMENTS *Xe3_G_DebugSurfaceLayout::DBG_ELEMENT_SIZE}, // dbg
        {offsetof(struct Xe3_G_DebugSurfaceLayout, ctx), Xe3_G_DebugSurfaceLayout::CONTEXT_ID_COUNT,
         Xe3_G_DebugSurfaceLayout::CONTEXT_ID_ELEMENTS *Xe3_G_DebugSurfaceLayout::CONTEXT_ID_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::CONTEXT_ID_ELEMENTS
             *Xe3_G_DebugSurfaceLayout::CONTEXT_ID_ELEMENT_SIZE}, // context id
        {offsetof(struct Xe3_G_DebugSurfaceLayout, dbg_reg), Xe3_G_DebugSurfaceLayout::DBG_REG_COUNT,
         Xe3_G_DebugSurfaceLayout::DBG_REG_ELEMENTS *Xe3_G_DebugSurfaceLayout::DBG_REG_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::DBG_REG_ELEMENTS *Xe3_G_DebugSurfaceLayout::DBG_REG_ELEMENT_SIZE}, // dbg registers
        {offsetof(struct Xe3_G_DebugSurfaceLayout, s), Xe3_G_DebugSurfaceLayout::SCALAR_REG_COUNT, // scalar registers
         Xe3_G_DebugSurfaceLayout::SCALAR_REG_ELEMENTS *Xe3_G_DebugSurfaceLayout::SCALAR_REG_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::SCALAR_REG_ELEMENTS *Xe3_G_DebugSurfaceLayout::SCALAR_REG_ELEMENT_SIZE},
        {offsetof(struct Xe3_G_DebugSurfaceLayout, msg), Xe3_G_DebugSurfaceLayout::MSG_COUNT,
         Xe3_G_DebugSurfaceLayout::MSG_ELEMENTS *Xe3_G_DebugSurfaceLayout::MSG_ELEMENT_SIZE * 8,
         Xe3_G_DebugSurfaceLayout::MSG_ELEMENTS *Xe3_G_DebugSurfaceLayout::MSG_ELEMENT_SIZE} // msg
    }
};


// Debug surface area for all XE2 architectures
struct Xe2DebugSurfaceLayout
{
    // The *_ALIGN fields below are padding of the SIP between
    // the registers set.
    static constexpr size_t GR_COUNT = 256;
    static constexpr size_t GR_ELEMENTS = 1;
    static constexpr size_t GR_ELEMENT_SIZE = 64;
    static constexpr size_t GR_ALIGN = 0;

    static constexpr size_t A0_COUNT = 1;
    static constexpr size_t A0_ELEMENTS = 16;
    static constexpr size_t A0_ELEMENT_SIZE = 2;
    static constexpr size_t A0_ALIGN = 0;

    static constexpr size_t F_COUNT = 4;
    static constexpr size_t F_ELEMENTS = 2;
    static constexpr size_t F_ELEMENT_SIZE = 2;
    static constexpr size_t F_ALIGN = 12;

    static constexpr size_t EXEC_MASK_COUNT = 1;
    static constexpr size_t EXEC_MASK_ELEMENTS = 1;
    static constexpr size_t EXEC_MASK_ELEMENT_SIZE = 4;
    static constexpr size_t EXEC_MASK_ALIGN = 0;

    static constexpr size_t SR_COUNT = 1;
    static constexpr size_t SR_ELEMENTS = 4;
    static constexpr size_t SR_ELEMENT_SIZE = 4;
    static constexpr size_t SR_ALIGN = 16;

    static constexpr size_t CR_COUNT = 1;
    static constexpr size_t CR_ELEMENTS = 3;
    static constexpr size_t CR_ELEMENT_SIZE = 4;
    static constexpr size_t CR_ALIGN = 20;

    static constexpr size_t N_COUNT = 1;
    static constexpr size_t N_ELEMENTS = 3;
    static constexpr size_t N_ELEMENT_SIZE = 4;
    static constexpr size_t N_ALIGN = 20;

    static constexpr size_t TDR_COUNT = 1;
    static constexpr size_t TDR_ELEMENTS = 8;
    static constexpr size_t TDR_ELEMENT_SIZE = 2;
    static constexpr size_t TDR_ALIGN = 16;

    static constexpr size_t ACC_COUNT = 4;
    static constexpr size_t ACC_ELEMENTS = 16;
    static constexpr size_t ACC_ELEMENT_SIZE = 4;
    static constexpr size_t ACC_ALIGN = 0;

    // Need to read msg registers to get mme register values
    static constexpr size_t MSG_COUNT = 2;
    static constexpr size_t MSG_ELEMENTS = 8;
    static constexpr size_t MSG_ELEMENT_SIZE = 4;
    static constexpr size_t MSG_ALIGN = 0;

    static constexpr size_t TM_COUNT = 1;
    static constexpr size_t TM_ELEMENTS = 5;
    static constexpr size_t TM_ELEMENT_SIZE = 4;
    static constexpr size_t TM_ALIGN = 12;

    static constexpr size_t CE_COUNT = 1;
    static constexpr size_t CE_ELEMENTS = 1;
    static constexpr size_t CE_ELEMENT_SIZE = 4;
    static constexpr size_t CE_ALIGN = 28;

    static constexpr size_t DBG_COUNT = 1;
    static constexpr size_t DBG_ELEMENTS = 1;
    static constexpr size_t DBG_ELEMENT_SIZE = 4;
    static constexpr size_t DBG_ALIGN = 0;

    static constexpr size_t VERSION_COUNT = 1;
    static constexpr size_t VERSION_ELEMENTS = 1;
    static constexpr size_t VERSION_ELEMENT_SIZE = 20;
    static constexpr size_t VERSION_ALIGN = 8;

    static constexpr size_t SIP_CMD_COUNT = 1;
    static constexpr size_t SIP_CMD_ELEMENTS = 1;
    static constexpr size_t SIP_CMD_ELEMENT_SIZE = 128;
    static constexpr size_t SIP_CMD_ALIGN = 0;

    static constexpr size_t CONTEXT_ID_COUNT = 1;
    static constexpr size_t CONTEXT_ID_ELEMENTS = 1;
    static constexpr size_t CONTEXT_ID_ELEMENT_SIZE = 8;
    static constexpr size_t CONTEXT_ID_ALIGN = 24;

    static constexpr size_t DBG_REG_COUNT = 1;
    static constexpr size_t DBG_REG_ELEMENTS = 2;
    static constexpr size_t DBG_REG_ELEMENT_SIZE = 4;
    static constexpr size_t DBG_REG_ALIGN = 24;


    // Number of Registers: 3
    // Elements:    16 or 2
    // Element Size:    32 bits
    // fc0.0-fc0.15
    // fc1.0-fc1.15
    // fc2.0 fc2.1, All other encodings like fc2.2 to f2.15 are reserved.
    static constexpr size_t FC_COUNT = 3;
    static constexpr size_t FC_ELEMENTS = 16;
    static constexpr size_t FC_ELEMENT_SIZE = 4;
    static constexpr size_t FC_ALIGN = 0;

    static constexpr size_t Xe2_STATE_SAVE_AREA_SIZE = 0x4400;

    uint8_t grf[GR_COUNT * GR_ELEMENTS * GR_ELEMENT_SIZE + GR_ALIGN];
    uint8_t a0[A0_COUNT * A0_ELEMENTS * A0_ELEMENT_SIZE + A0_ALIGN];
    uint8_t f[F_COUNT * F_ELEMENTS * F_ELEMENT_SIZE + F_ALIGN];
    uint8_t execmask[EXEC_MASK_COUNT * EXEC_MASK_ELEMENTS * EXEC_MASK_ELEMENT_SIZE + EXEC_MASK_ALIGN];
    uint8_t sr[SR_COUNT * SR_ELEMENTS * SR_ELEMENT_SIZE + SR_ALIGN];
    uint8_t cr[CR_COUNT * CR_ELEMENTS * CR_ELEMENT_SIZE + CR_ALIGN];
    uint8_t n[N_COUNT * N_ELEMENTS * N_ELEMENT_SIZE + N_ALIGN];
    uint8_t tdr[TDR_COUNT * TDR_ELEMENTS * TDR_ELEMENT_SIZE + TDR_ALIGN];
    uint8_t acc[ACC_COUNT * ACC_ELEMENTS * ACC_ELEMENT_SIZE + ACC_ALIGN];
    uint8_t mme[MSG_COUNT * MSG_ELEMENTS * MSG_ELEMENT_SIZE + MSG_ALIGN];
    uint8_t tm[TM_COUNT * TM_ELEMENTS * TM_ELEMENT_SIZE + TM_ALIGN];
    uint8_t ce[CE_COUNT * CE_ELEMENTS * CE_ELEMENT_SIZE + CE_ALIGN];
    uint8_t dbg[DBG_COUNT * DBG_ELEMENTS * DBG_ELEMENT_SIZE + DBG_ALIGN];
    uint8_t version[VERSION_COUNT * VERSION_ELEMENTS * VERSION_ELEMENT_SIZE + VERSION_ALIGN];
    uint8_t sip_cmd[SIP_CMD_COUNT * SIP_CMD_ELEMENTS * SIP_CMD_ELEMENT_SIZE + SIP_CMD_ALIGN];
    uint8_t ctx[CONTEXT_ID_COUNT * CONTEXT_ID_ELEMENTS * CONTEXT_ID_ELEMENT_SIZE + CONTEXT_ID_ALIGN];
    uint8_t dbg_reg[DBG_REG_COUNT * DBG_REG_ELEMENTS * DBG_REG_ELEMENT_SIZE + DBG_REG_ALIGN];
    uint8_t fc[FC_COUNT * FC_ELEMENTS * FC_ELEMENT_SIZE + FC_ALIGN];
};

struct StateSaveAreaHeader Xe2SIPCSRDebugBindlessDebugHeader =
{
    {"tssarea", 0, {2, 2, 0}, sizeof(StateSaveAreaHeader) / 8, {0, 0, 0}}, // versionHeader
    {
        // regHeader
        0,                                               // num_slices
        0,                                               // num_subslices_per_slice
        0,                                               // num_eus_per_subslice
        0,                                               // num_threads_per_eu
        0,                                               // state_area_offset
        Xe2DebugSurfaceLayout::Xe2_STATE_SAVE_AREA_SIZE, // state_save_size
        0,                                               // slm_area_offset
        0,                                               // slm_bank_size
        0,                                               // slm_bank_valid
        offsetof(struct Xe2DebugSurfaceLayout, version), // sr_magic_offset
        {offsetof(struct Xe2DebugSurfaceLayout, grf), Xe2DebugSurfaceLayout::GR_COUNT,
         Xe2DebugSurfaceLayout::GR_ELEMENTS *Xe2DebugSurfaceLayout::GR_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::GR_ELEMENTS *Xe2DebugSurfaceLayout::GR_ELEMENT_SIZE}, // grf
        {offsetof(struct Xe2DebugSurfaceLayout, a0), Xe2DebugSurfaceLayout::A0_COUNT,
         Xe2DebugSurfaceLayout::A0_ELEMENTS *Xe2DebugSurfaceLayout::A0_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::A0_ELEMENTS *Xe2DebugSurfaceLayout::A0_ELEMENT_SIZE}, // addr
        {offsetof(struct Xe2DebugSurfaceLayout, f), Xe2DebugSurfaceLayout::F_COUNT,
         Xe2DebugSurfaceLayout::F_ELEMENTS *Xe2DebugSurfaceLayout::F_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::F_ELEMENTS *Xe2DebugSurfaceLayout::F_ELEMENT_SIZE}, // flag
        {offsetof(struct Xe2DebugSurfaceLayout, execmask), Xe2DebugSurfaceLayout::EXEC_MASK_COUNT,
         Xe2DebugSurfaceLayout::EXEC_MASK_ELEMENTS *Xe2DebugSurfaceLayout::EXEC_MASK_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::EXEC_MASK_ELEMENTS *Xe2DebugSurfaceLayout::EXEC_MASK_ELEMENT_SIZE}, // emask
        {offsetof(struct Xe2DebugSurfaceLayout, sr), Xe2DebugSurfaceLayout::SR_COUNT,
         Xe2DebugSurfaceLayout::SR_ELEMENTS *Xe2DebugSurfaceLayout::SR_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::SR_ELEMENTS *Xe2DebugSurfaceLayout::SR_ELEMENT_SIZE}, // sr
        {offsetof(struct Xe2DebugSurfaceLayout, cr), Xe2DebugSurfaceLayout::CR_COUNT,
         Xe2DebugSurfaceLayout::CR_ELEMENTS *Xe2DebugSurfaceLayout::CR_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::CR_ELEMENTS *Xe2DebugSurfaceLayout::CR_ELEMENT_SIZE}, // cr
        {offsetof(struct Xe2DebugSurfaceLayout, n), Xe2DebugSurfaceLayout::N_COUNT,
         Xe2DebugSurfaceLayout::N_ELEMENTS *Xe2DebugSurfaceLayout::N_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::N_ELEMENTS *Xe2DebugSurfaceLayout::N_ELEMENT_SIZE}, // notification
        {offsetof(struct Xe2DebugSurfaceLayout, tdr), Xe2DebugSurfaceLayout::TDR_COUNT,
         Xe2DebugSurfaceLayout::TDR_ELEMENTS *Xe2DebugSurfaceLayout::TDR_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::TDR_ELEMENTS *Xe2DebugSurfaceLayout::TDR_ELEMENT_SIZE}, // tdr
        {offsetof(struct Xe2DebugSurfaceLayout, acc), Xe2DebugSurfaceLayout::ACC_COUNT,
         Xe2DebugSurfaceLayout::ACC_ELEMENTS *Xe2DebugSurfaceLayout::ACC_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::ACC_ELEMENTS *Xe2DebugSurfaceLayout::ACC_ELEMENT_SIZE}, // acc
        {offsetof(struct Xe2DebugSurfaceLayout, mme), Xe2DebugSurfaceLayout::MSG_COUNT,
         Xe2DebugSurfaceLayout::MSG_ELEMENTS *Xe2DebugSurfaceLayout::MSG_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::MSG_ELEMENTS *Xe2DebugSurfaceLayout::MSG_ELEMENT_SIZE}, // mme
        {offsetof(struct Xe2DebugSurfaceLayout, ce), Xe2DebugSurfaceLayout::CE_COUNT,
         Xe2DebugSurfaceLayout::CE_ELEMENTS *Xe2DebugSurfaceLayout::CE_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::CE_ELEMENTS *Xe2DebugSurfaceLayout::CE_ELEMENT_SIZE}, // ce
        {0, 0, 0, 0},
        {offsetof(struct Xe2DebugSurfaceLayout, sip_cmd), Xe2DebugSurfaceLayout::SIP_CMD_COUNT,
         Xe2DebugSurfaceLayout::SIP_CMD_ELEMENTS *Xe2DebugSurfaceLayout::SIP_CMD_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::SIP_CMD_ELEMENTS *Xe2DebugSurfaceLayout::SIP_CMD_ELEMENT_SIZE}, // cmd
        {offsetof(struct Xe2DebugSurfaceLayout, tm), Xe2DebugSurfaceLayout::TM_COUNT,
         Xe2DebugSurfaceLayout::TM_ELEMENTS *Xe2DebugSurfaceLayout::TM_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::TM_ELEMENTS *Xe2DebugSurfaceLayout::TM_ELEMENT_SIZE}, // tm
        {offsetof(struct Xe2DebugSurfaceLayout, fc), Xe2DebugSurfaceLayout::FC_COUNT,
         Xe2DebugSurfaceLayout::FC_ELEMENTS *Xe2DebugSurfaceLayout::FC_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::FC_ELEMENTS *Xe2DebugSurfaceLayout::FC_ELEMENT_SIZE}, // FC
        {offsetof(struct Xe2DebugSurfaceLayout, dbg), Xe2DebugSurfaceLayout::DBG_COUNT,
         Xe2DebugSurfaceLayout::DBG_ELEMENTS *Xe2DebugSurfaceLayout::DBG_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::DBG_ELEMENTS *Xe2DebugSurfaceLayout::DBG_ELEMENT_SIZE}, // dbg
        {offsetof(struct Xe2DebugSurfaceLayout, ctx), Xe2DebugSurfaceLayout::CONTEXT_ID_COUNT,
         Xe2DebugSurfaceLayout::CONTEXT_ID_ELEMENTS *Xe2DebugSurfaceLayout::CONTEXT_ID_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::CONTEXT_ID_ELEMENTS *Xe2DebugSurfaceLayout::CONTEXT_ID_ELEMENT_SIZE}, // context id
        {offsetof(struct Xe2DebugSurfaceLayout, dbg_reg), Xe2DebugSurfaceLayout::DBG_REG_COUNT,
         Xe2DebugSurfaceLayout::DBG_REG_ELEMENTS *Xe2DebugSurfaceLayout::DBG_REG_ELEMENT_SIZE * 8,
         Xe2DebugSurfaceLayout::DBG_REG_ELEMENTS *Xe2DebugSurfaceLayout::DBG_REG_ELEMENT_SIZE}, // dbg registers
    }
};


// wmtp Xe2 SIP
struct StateSaveAreaHeaderV4 Xe2SIP_WMTP_CSRDebugBindlessDebugHeader =
{
    {"tssarea", 0, {4, 0, 0}, sizeof(StateSaveAreaHeaderV4) / 8, {0, 0, 0}}, // versionHeader
    0 // total_wmtp_data_size
};

// Debug surface area for all XeHPC+ architectures
struct XeHPCDebugSurfaceLayout
{
    // The *_ALIGN fields below are padding of the SIP between
    // the registers set.
    static constexpr size_t GR_COUNT = 256;
    static constexpr size_t GR_ELEMENTS = 1;
    static constexpr size_t GR_ELEMENT_SIZE = 64;
    static constexpr size_t GR_ALIGN = 0;

    static constexpr size_t A0_COUNT = 1;
    static constexpr size_t A0_ELEMENTS = 16;
    static constexpr size_t A0_ELEMENT_SIZE = 2;
    static constexpr size_t A0_ALIGN = 0;

    static constexpr size_t F_COUNT = 4;
    static constexpr size_t F_ELEMENTS = 2;
    static constexpr size_t F_ELEMENT_SIZE = 2;
    static constexpr size_t F_ALIGN = 12;

    static constexpr size_t EXEC_MASK_COUNT = 1;
    static constexpr size_t EXEC_MASK_ELEMENTS = 1;
    static constexpr size_t EXEC_MASK_ELEMENT_SIZE = 4;
    static constexpr size_t EXEC_MASK_ALIGN = 0;

    static constexpr size_t SR_COUNT = 1;
    static constexpr size_t SR_ELEMENTS = 4;
    static constexpr size_t SR_ELEMENT_SIZE = 4;
    static constexpr size_t SR_ALIGN = 16;

    static constexpr size_t CR_COUNT = 1;
    static constexpr size_t CR_ELEMENTS = 4;
    static constexpr size_t CR_ELEMENT_SIZE = 4;
    static constexpr size_t CR_ALIGN = 16;

    static constexpr size_t IP_COUNT = 1;
    static constexpr size_t IP_ELEMENTS = 1;
    static constexpr size_t IP_ELEMENT_SIZE = 4;
    static constexpr size_t IP_ALIGN = 28;

    static constexpr size_t N_COUNT = 1;
    static constexpr size_t N_ELEMENTS = 3;
    static constexpr size_t N_ELEMENT_SIZE = 4;
    static constexpr size_t N_ALIGN = 20;

    static constexpr size_t TDR_COUNT = 1;
    static constexpr size_t TDR_ELEMENTS = 8;
    static constexpr size_t TDR_ELEMENT_SIZE = 2;
    static constexpr size_t TDR_ALIGN = 48;

    static constexpr size_t ACC_COUNT = 4;
    static constexpr size_t ACC_ELEMENTS = 16;
    static constexpr size_t ACC_ELEMENT_SIZE = 4;
    static constexpr size_t ACC_ALIGN = 0;

    static constexpr size_t MME_COUNT = 8;
    static constexpr size_t MME_ELEMENTS = 16;
    static constexpr size_t MME_ELEMENT_SIZE = 4;
    static constexpr size_t MME_ALIGN = 0;

    static constexpr size_t TM_COUNT = 1;
    static constexpr size_t TM_ELEMENTS = 5;
    static constexpr size_t TM_ELEMENT_SIZE = 4;
    static constexpr size_t TM_ALIGN = 12;

    static constexpr size_t CE_COUNT = 1;
    static constexpr size_t CE_ELEMENTS = 1;
    static constexpr size_t CE_ELEMENT_SIZE = 4;
    static constexpr size_t CE_ALIGN = 12;

    static constexpr size_t SP_COUNT = 1;
    static constexpr size_t SP_ELEMENTS = 2;
    static constexpr size_t SP_ELEMENT_SIZE = 8;
    static constexpr size_t SP_ALIGN = 0;

    static constexpr size_t DBG_COUNT = 1;
    static constexpr size_t DBG_ELEMENTS = 1;
    static constexpr size_t DBG_ELEMENT_SIZE = 4;
    static constexpr size_t DBG_ALIGN = 0;

    static constexpr size_t VERSION_COUNT = 1;
    static constexpr size_t VERSION_ELEMENTS = 1;
    static constexpr size_t VERSION_ELEMENT_SIZE = 20;
    static constexpr size_t VERSION_ALIGN = 8;

    static constexpr size_t SIP_CMD_COUNT = 1;
    static constexpr size_t SIP_CMD_ELEMENTS = 1;
    static constexpr size_t SIP_CMD_ELEMENT_SIZE = 128;
    static constexpr size_t SIP_CMD_ALIGN = 0;

    static constexpr size_t CONTEXT_ID_COUNT = 1;
    static constexpr size_t CONTEXT_ID_ELEMENTS = 1;
    static constexpr size_t CONTEXT_ID_ELEMENT_SIZE = 8;
    static constexpr size_t CONTEXT_ID_ALIGN = 24;

    static constexpr size_t DBG_REG_COUNT = 1;
    static constexpr size_t DBG_REG_ELEMENTS = 2;
    static constexpr size_t DBG_REG_ELEMENT_SIZE = 4;
    static constexpr size_t DBG_REG_ALIGN = 24;

    uint8_t grf[GR_COUNT * GR_ELEMENTS * GR_ELEMENT_SIZE + GR_ALIGN];
    uint8_t a0[A0_COUNT * A0_ELEMENTS * A0_ELEMENT_SIZE + A0_ALIGN];
    uint8_t f[F_COUNT * F_ELEMENTS * F_ELEMENT_SIZE + F_ALIGN];
    uint8_t execmask[EXEC_MASK_COUNT * EXEC_MASK_ELEMENTS * EXEC_MASK_ELEMENT_SIZE + EXEC_MASK_ALIGN];
    uint8_t sr[SR_COUNT * SR_ELEMENTS * SR_ELEMENT_SIZE + SR_ALIGN];
    uint8_t cr[CR_COUNT * CR_ELEMENTS * CR_ELEMENT_SIZE + CR_ALIGN];
    uint8_t ip[IP_COUNT * IP_ELEMENTS * IP_ELEMENT_SIZE + IP_ALIGN];
    uint8_t n[N_COUNT * N_ELEMENTS * N_ELEMENT_SIZE + N_ALIGN];
    uint8_t tdr[TDR_COUNT * TDR_ELEMENTS * TDR_ELEMENT_SIZE + TDR_ALIGN];
    uint8_t acc[ACC_COUNT * ACC_ELEMENTS * ACC_ELEMENT_SIZE + ACC_ALIGN];
    uint8_t mme[MME_COUNT * MME_ELEMENTS * MME_ELEMENT_SIZE + MME_ALIGN];
    uint8_t tm[TM_COUNT * TM_ELEMENTS * TM_ELEMENT_SIZE + TM_ALIGN];
    uint8_t ce[CE_COUNT * CE_ELEMENTS * CE_ELEMENT_SIZE + CE_ALIGN];
    uint8_t sp[SP_COUNT * SP_ELEMENTS * SP_ELEMENT_SIZE + SP_ALIGN];
    uint8_t dbg[DBG_COUNT * DBG_ELEMENTS * DBG_ELEMENT_SIZE + DBG_ALIGN];
    uint8_t version[VERSION_COUNT * VERSION_ELEMENTS * VERSION_ELEMENT_SIZE + VERSION_ALIGN];
    uint8_t sip_cmd[SIP_CMD_COUNT * SIP_CMD_ELEMENTS * SIP_CMD_ELEMENT_SIZE + SIP_CMD_ALIGN];
    uint8_t ctx[CONTEXT_ID_COUNT * CONTEXT_ID_ELEMENTS *CONTEXT_ID_ELEMENT_SIZE +CONTEXT_ID_ALIGN];
    uint8_t dbg_reg[DBG_REG_COUNT * DBG_REG_ELEMENTS * DBG_REG_ELEMENT_SIZE +DBG_REG_ALIGN];
};

static struct StateSaveAreaHeader XeHPCSIPCSRDebugBindlessDebugHeader =
{
    {"tssarea", 0, {2, 2, 0}, sizeof(StateSaveAreaHeader) / 8, {0, 0, 0}},  // versionHeader
    {
        // regHeader
        0,                               // num_slices
        0,                               // num_subslices_per_slice
        0,                               // num_eus_per_subslice
        0,                               // num_threads_per_eu
        0,                               // state_area_offset
        0x4600,                          // state_save_size
        0,                               // slm_area_offset
        0,                               // slm_bank_size
        0,                               // slm_bank_valid
        offsetof(struct XeHPCDebugSurfaceLayout, version),  // sr_magic_offset
        {offsetof(struct XeHPCDebugSurfaceLayout, grf), XeHPCDebugSurfaceLayout::GR_COUNT,
         XeHPCDebugSurfaceLayout::GR_ELEMENTS* XeHPCDebugSurfaceLayout::GR_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::GR_ELEMENTS* XeHPCDebugSurfaceLayout::GR_ELEMENT_SIZE},  // grf
        {offsetof(struct XeHPCDebugSurfaceLayout, a0), XeHPCDebugSurfaceLayout::A0_COUNT,
         XeHPCDebugSurfaceLayout::A0_ELEMENTS* XeHPCDebugSurfaceLayout::A0_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::A0_ELEMENTS* XeHPCDebugSurfaceLayout::A0_ELEMENT_SIZE},  // addr
        {offsetof(struct XeHPCDebugSurfaceLayout, f), XeHPCDebugSurfaceLayout::F_COUNT,
         XeHPCDebugSurfaceLayout::F_ELEMENTS* XeHPCDebugSurfaceLayout::F_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::F_ELEMENTS* XeHPCDebugSurfaceLayout::F_ELEMENT_SIZE},  // flag
        {offsetof(struct XeHPCDebugSurfaceLayout, execmask), XeHPCDebugSurfaceLayout::EXEC_MASK_COUNT,
         XeHPCDebugSurfaceLayout::EXEC_MASK_ELEMENTS* XeHPCDebugSurfaceLayout::EXEC_MASK_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::EXEC_MASK_ELEMENTS* XeHPCDebugSurfaceLayout::EXEC_MASK_ELEMENT_SIZE},  // emask
        {offsetof(struct XeHPCDebugSurfaceLayout, sr), XeHPCDebugSurfaceLayout::SR_COUNT,
         XeHPCDebugSurfaceLayout::SR_ELEMENTS* XeHPCDebugSurfaceLayout::SR_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::SR_ELEMENTS* XeHPCDebugSurfaceLayout::SR_ELEMENT_SIZE},  // sr
        {offsetof(struct XeHPCDebugSurfaceLayout, cr), XeHPCDebugSurfaceLayout::CR_COUNT,
         XeHPCDebugSurfaceLayout::CR_ELEMENTS* XeHPCDebugSurfaceLayout::CR_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::CR_ELEMENTS* XeHPCDebugSurfaceLayout::CR_ELEMENT_SIZE},  // cr
        {offsetof(struct XeHPCDebugSurfaceLayout, n), XeHPCDebugSurfaceLayout::N_COUNT,
         XeHPCDebugSurfaceLayout::N_ELEMENTS* XeHPCDebugSurfaceLayout::N_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::N_ELEMENTS* XeHPCDebugSurfaceLayout::N_ELEMENT_SIZE},  // notification
        {offsetof(struct XeHPCDebugSurfaceLayout, tdr), XeHPCDebugSurfaceLayout::TDR_COUNT,
         XeHPCDebugSurfaceLayout::TDR_ELEMENTS* XeHPCDebugSurfaceLayout::TDR_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::TDR_ELEMENTS* XeHPCDebugSurfaceLayout::TDR_ELEMENT_SIZE},  // tdr
        {offsetof(struct XeHPCDebugSurfaceLayout, acc), XeHPCDebugSurfaceLayout::ACC_COUNT,
         XeHPCDebugSurfaceLayout::ACC_ELEMENTS* XeHPCDebugSurfaceLayout::ACC_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::ACC_ELEMENTS* XeHPCDebugSurfaceLayout::ACC_ELEMENT_SIZE},  // acc
        {offsetof(struct XeHPCDebugSurfaceLayout, mme), XeHPCDebugSurfaceLayout::MME_COUNT,
         XeHPCDebugSurfaceLayout::MME_ELEMENTS* XeHPCDebugSurfaceLayout::MME_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::MME_ELEMENTS* XeHPCDebugSurfaceLayout::MME_ELEMENT_SIZE},  // mme
        {offsetof(struct XeHPCDebugSurfaceLayout, ce), XeHPCDebugSurfaceLayout::CE_COUNT,
         XeHPCDebugSurfaceLayout::CE_ELEMENTS* XeHPCDebugSurfaceLayout::CE_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::CE_ELEMENTS* XeHPCDebugSurfaceLayout::CE_ELEMENT_SIZE},  // ce
        {offsetof(struct XeHPCDebugSurfaceLayout, sp), XeHPCDebugSurfaceLayout::SP_COUNT,
         XeHPCDebugSurfaceLayout::SP_ELEMENTS* XeHPCDebugSurfaceLayout::SP_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::SP_ELEMENTS* XeHPCDebugSurfaceLayout::SP_ELEMENT_SIZE},  // sp
        {offsetof(struct XeHPCDebugSurfaceLayout, sip_cmd), XeHPCDebugSurfaceLayout::SIP_CMD_COUNT,
         XeHPCDebugSurfaceLayout::SIP_CMD_ELEMENTS* XeHPCDebugSurfaceLayout::SIP_CMD_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::SIP_CMD_ELEMENTS* XeHPCDebugSurfaceLayout::SIP_CMD_ELEMENT_SIZE},  // cmd
        {offsetof(struct XeHPCDebugSurfaceLayout, tm), XeHPCDebugSurfaceLayout::TM_COUNT,
         XeHPCDebugSurfaceLayout::TM_ELEMENTS* XeHPCDebugSurfaceLayout::TM_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::TM_ELEMENTS* XeHPCDebugSurfaceLayout::TM_ELEMENT_SIZE},  // tm
        {0, 0, 0, 0},                                  // FC
        {offsetof(struct XeHPCDebugSurfaceLayout, dbg), XeHPCDebugSurfaceLayout::DBG_COUNT,
         XeHPCDebugSurfaceLayout::DBG_ELEMENTS* XeHPCDebugSurfaceLayout::DBG_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::DBG_ELEMENTS* XeHPCDebugSurfaceLayout::DBG_ELEMENT_SIZE},  // dbg
        {offsetof(struct XeHPCDebugSurfaceLayout, ctx), XeHPCDebugSurfaceLayout::CONTEXT_ID_COUNT,
         XeHPCDebugSurfaceLayout::CONTEXT_ID_ELEMENTS* XeHPCDebugSurfaceLayout::CONTEXT_ID_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::CONTEXT_ID_ELEMENTS* XeHPCDebugSurfaceLayout::CONTEXT_ID_ELEMENT_SIZE}, // context id
        {offsetof(struct XeHPCDebugSurfaceLayout, dbg_reg), XeHPCDebugSurfaceLayout::DBG_REG_COUNT,
         XeHPCDebugSurfaceLayout::DBG_REG_ELEMENTS* XeHPCDebugSurfaceLayout::DBG_REG_ELEMENT_SIZE * 8,
         XeHPCDebugSurfaceLayout::DBG_REG_ELEMENTS* XeHPCDebugSurfaceLayout::DBG_REG_ELEMENT_SIZE}, // dbg registers
    }
};

// Debug surface area for all GEN8+ architectures
struct DebugSurfaceLayout
{
    // The *_ALIGN fields below are padding of the SIP between
    // the registers set.
    static constexpr size_t GR_COUNT = 128;
    static constexpr size_t GR_ELEMENTS = 1;
    static constexpr size_t GR_ELEMENT_SIZE = 32;
    static constexpr size_t GR_ALIGN = 0;

    static constexpr size_t A0_COUNT = 1;
    static constexpr size_t A0_ELEMENTS = 16;
    static constexpr size_t A0_ELEMENT_SIZE = 2;
    static constexpr size_t A0_ALIGN = 0;

    static constexpr size_t F_COUNT = 2;
    static constexpr size_t F_ELEMENTS = 2;
    static constexpr size_t F_ELEMENT_SIZE = 2;
    static constexpr size_t F_ALIGN = 20;

    static constexpr size_t EXEC_MASK_COUNT = 1;
    static constexpr size_t EXEC_MASK_ELEMENTS = 1;
    static constexpr size_t EXEC_MASK_ELEMENT_SIZE = 4;
    static constexpr size_t EXEC_MASK_ALIGN = 0;

    static constexpr size_t SR_COUNT = 1;
    static constexpr size_t SR_ELEMENTS = 4;
    static constexpr size_t SR_ELEMENT_SIZE = 4;
    static constexpr size_t SR_ALIGN = 16;

    static constexpr size_t CR_COUNT = 1;
    static constexpr size_t CR_ELEMENTS = 4;
    static constexpr size_t CR_ELEMENT_SIZE = 4;
    static constexpr size_t CR_ALIGN = 16;

    static constexpr size_t IP_COUNT = 1;
    static constexpr size_t IP_ELEMENTS = 1;
    static constexpr size_t IP_ELEMENT_SIZE = 4;
    static constexpr size_t IP_ALIGN = 28;

    static constexpr size_t N_COUNT = 1;
    static constexpr size_t N_ELEMENTS = 3;
    static constexpr size_t N_ELEMENT_SIZE = 4;
    static constexpr size_t N_ALIGN = 20;

    static constexpr size_t TDR_COUNT = 1;
    static constexpr size_t TDR_ELEMENTS = 8;
    static constexpr size_t TDR_ELEMENT_SIZE = 2;
    static constexpr size_t TDR_ALIGN = 16;

    static constexpr size_t ACC_COUNT = 10;
    static constexpr size_t ACC_ELEMENTS = 8;
    static constexpr size_t ACC_ELEMENT_SIZE = 4;
    static constexpr size_t ACC_ALIGN = 0;

    static constexpr size_t TM_COUNT = 1;
    static constexpr size_t TM_ELEMENTS = 4;
    static constexpr size_t TM_ELEMENT_SIZE = 4;
    static constexpr size_t TM_ALIGN = 16;

    static constexpr size_t CE_COUNT = 1;
    static constexpr size_t CE_ELEMENTS = 1;
    static constexpr size_t CE_ELEMENT_SIZE = 4;
    static constexpr size_t CE_ALIGN = 28;

    static constexpr size_t SP_COUNT = 1;
    static constexpr size_t SP_ELEMENTS = 2;
    static constexpr size_t SP_ELEMENT_SIZE = 8;
    static constexpr size_t SP_ALIGN = 16;

    static constexpr size_t DBG_COUNT = 1;
    static constexpr size_t DBG_ELEMENTS = 1;
    static constexpr size_t DBG_ELEMENT_SIZE = 4;
    static constexpr size_t DBG_ALIGN = 0;

    static constexpr size_t VERSION_COUNT = 1;
    static constexpr size_t VERSION_ELEMENTS = 1;
    static constexpr size_t VERSION_ELEMENT_SIZE = 20;
    static constexpr size_t VERSION_ALIGN = 8;

    static constexpr size_t SIP_CMD_COUNT = 1;
    static constexpr size_t SIP_CMD_ELEMENTS = 1;
    static constexpr size_t SIP_CMD_ELEMENT_SIZE = 128;
    static constexpr size_t SIP_CMD_ALIGN = 0;

    uint8_t grf[GR_COUNT * GR_ELEMENTS * GR_ELEMENT_SIZE + GR_ALIGN];
    uint8_t a0[A0_COUNT * A0_ELEMENTS * A0_ELEMENT_SIZE + A0_ALIGN];
    uint8_t f[F_COUNT * F_ELEMENTS * F_ELEMENT_SIZE + F_ALIGN];
    uint8_t execmask[EXEC_MASK_COUNT * EXEC_MASK_ELEMENTS * EXEC_MASK_ELEMENT_SIZE + EXEC_MASK_ALIGN];
    uint8_t sr[SR_COUNT * SR_ELEMENTS * SR_ELEMENT_SIZE + SR_ALIGN];
    uint8_t cr[CR_COUNT * CR_ELEMENTS * CR_ELEMENT_SIZE + CR_ALIGN];
    uint8_t ip[IP_COUNT * IP_ELEMENTS * IP_ELEMENT_SIZE + IP_ALIGN];
    uint8_t n[N_COUNT * N_ELEMENTS * N_ELEMENT_SIZE + N_ALIGN];
    uint8_t tdr[TDR_COUNT * TDR_ELEMENTS * TDR_ELEMENT_SIZE + TDR_ALIGN];
    uint8_t acc[ACC_COUNT * ACC_ELEMENTS * ACC_ELEMENT_SIZE + ACC_ALIGN];
    uint8_t tm[TM_COUNT * TM_ELEMENTS * TM_ELEMENT_SIZE + TM_ALIGN];
    uint8_t ce[CE_COUNT * CE_ELEMENTS * CE_ELEMENT_SIZE + CE_ALIGN];
    uint8_t sp[SP_COUNT * SP_ELEMENTS * SP_ELEMENT_SIZE + SP_ALIGN];
    uint8_t dbg[DBG_COUNT * DBG_ELEMENTS * DBG_ELEMENT_SIZE + DBG_ALIGN];
    uint8_t version[VERSION_COUNT * VERSION_ELEMENTS * VERSION_ELEMENT_SIZE + VERSION_ALIGN];
    uint8_t sip_cmd[SIP_CMD_COUNT * SIP_CMD_ELEMENTS * SIP_CMD_ELEMENT_SIZE + SIP_CMD_ALIGN];
};

static struct StateSaveAreaHeader Gen12SIPCSRDebugBindlessDebugHeader =
{
    {"tssarea", 0, {2, 0, 0}, (offsetof(intelgt_state_save_area, ctx) + sizeof(StateSaveArea)) / 8, {0, 0, 0}},  // versionHeader
    {
        // regHeader
        0,                               // num_slices
        0,                               // num_subslices_per_slice
        0,                               // num_eus_per_subslice
        0,                               // num_threads_per_eu
        0,                               // state_area_offset
        0x1800,                          // state_save_size
        0,                               // slm_area_offset
        0,                               // slm_bank_size
        0,                               // slm_bank_valid
        offsetof(struct DebugSurfaceLayout, version),  // sr_magic_offset
        {offsetof(struct DebugSurfaceLayout, grf), DebugSurfaceLayout::GR_COUNT,
         DebugSurfaceLayout::GR_ELEMENTS* DebugSurfaceLayout::GR_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::GR_ELEMENTS* DebugSurfaceLayout::GR_ELEMENT_SIZE},  // grf
        {offsetof(struct DebugSurfaceLayout, a0), DebugSurfaceLayout::A0_COUNT,
         DebugSurfaceLayout::A0_ELEMENTS* DebugSurfaceLayout::A0_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::A0_ELEMENTS* DebugSurfaceLayout::A0_ELEMENT_SIZE},  // addr
        {offsetof(struct DebugSurfaceLayout, f), DebugSurfaceLayout::F_COUNT,
         DebugSurfaceLayout::F_ELEMENTS* DebugSurfaceLayout::F_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::F_ELEMENTS* DebugSurfaceLayout::F_ELEMENT_SIZE},  // flag
        {offsetof(struct DebugSurfaceLayout, execmask), DebugSurfaceLayout::EXEC_MASK_COUNT,
         DebugSurfaceLayout::EXEC_MASK_ELEMENTS* DebugSurfaceLayout::EXEC_MASK_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::EXEC_MASK_ELEMENTS* DebugSurfaceLayout::EXEC_MASK_ELEMENT_SIZE},  // emask
        {offsetof(struct DebugSurfaceLayout, sr), DebugSurfaceLayout::SR_COUNT,
         DebugSurfaceLayout::SR_ELEMENTS* DebugSurfaceLayout::SR_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::SR_ELEMENTS* DebugSurfaceLayout::SR_ELEMENT_SIZE},  // sr
        {offsetof(struct DebugSurfaceLayout, cr), DebugSurfaceLayout::CR_COUNT,
         DebugSurfaceLayout::CR_ELEMENTS* DebugSurfaceLayout::CR_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::CR_ELEMENTS* DebugSurfaceLayout::CR_ELEMENT_SIZE},  // cr
        {offsetof(struct DebugSurfaceLayout, n), DebugSurfaceLayout::N_COUNT,
         DebugSurfaceLayout::N_ELEMENTS* DebugSurfaceLayout::N_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::N_ELEMENTS* DebugSurfaceLayout::N_ELEMENT_SIZE},  // notification
        {offsetof(struct DebugSurfaceLayout, tdr), DebugSurfaceLayout::TDR_COUNT,
         DebugSurfaceLayout::TDR_ELEMENTS* DebugSurfaceLayout::TDR_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::TDR_ELEMENTS* DebugSurfaceLayout::TDR_ELEMENT_SIZE},  // tdr
        {offsetof(struct DebugSurfaceLayout, acc), DebugSurfaceLayout::ACC_COUNT,
         DebugSurfaceLayout::ACC_ELEMENTS* DebugSurfaceLayout::ACC_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::ACC_ELEMENTS* DebugSurfaceLayout::ACC_ELEMENT_SIZE},  // acc
        {0, 0, 0, 0},                                  // mme
        {offsetof(struct DebugSurfaceLayout, ce), DebugSurfaceLayout::CE_COUNT,
         DebugSurfaceLayout::CE_ELEMENTS* DebugSurfaceLayout::CE_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::CE_ELEMENTS* DebugSurfaceLayout::CE_ELEMENT_SIZE},  // ce
        {offsetof(struct DebugSurfaceLayout, sp), DebugSurfaceLayout::SP_COUNT,
         DebugSurfaceLayout::SP_ELEMENTS* DebugSurfaceLayout::SP_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::SP_ELEMENTS* DebugSurfaceLayout::SP_ELEMENT_SIZE},  // sp
        {offsetof(struct DebugSurfaceLayout, sip_cmd), DebugSurfaceLayout::SIP_CMD_COUNT,
         DebugSurfaceLayout::SIP_CMD_ELEMENTS* DebugSurfaceLayout::SIP_CMD_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::SIP_CMD_ELEMENTS* DebugSurfaceLayout::SIP_CMD_ELEMENT_SIZE},  // cmd
        {offsetof(struct DebugSurfaceLayout, tm), DebugSurfaceLayout::TM_COUNT,
         DebugSurfaceLayout::TM_ELEMENTS* DebugSurfaceLayout::TM_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::TM_ELEMENTS* DebugSurfaceLayout::TM_ELEMENT_SIZE},  // tm
        {0, 0, 0, 0},                                  // FC
        {offsetof(struct DebugSurfaceLayout, dbg), DebugSurfaceLayout::DBG_COUNT,
         DebugSurfaceLayout::DBG_ELEMENTS* DebugSurfaceLayout::DBG_ELEMENT_SIZE * 8,
         DebugSurfaceLayout::DBG_ELEMENTS* DebugSurfaceLayout::DBG_ELEMENT_SIZE}  // dbg
    }
};

// Debug surface area for all DG2 architectures
struct XeHPGDebugSurfaceLayout
{
    // The *_ALIGN fields below are padding of the SIP between
    // the registers set.
    static constexpr size_t GR_COUNT = 256;
    static constexpr size_t GR_ELEMENTS = 1;
    static constexpr size_t GR_ELEMENT_SIZE = 32;
    static constexpr size_t GR_ALIGN = 0;

    static constexpr size_t A0_COUNT = 1;
    static constexpr size_t A0_ELEMENTS = 16;
    static constexpr size_t A0_ELEMENT_SIZE = 2;
    static constexpr size_t A0_ALIGN = 0;

    static constexpr size_t F_COUNT = 2;
    static constexpr size_t F_ELEMENTS = 2;
    static constexpr size_t F_ELEMENT_SIZE = 2;
    static constexpr size_t F_ALIGN = 20;

    static constexpr size_t EXEC_MASK_COUNT = 1;
    static constexpr size_t EXEC_MASK_ELEMENTS = 1;
    static constexpr size_t EXEC_MASK_ELEMENT_SIZE = 4;
    static constexpr size_t EXEC_MASK_ALIGN = 0;

    static constexpr size_t SR_COUNT = 1;
    static constexpr size_t SR_ELEMENTS = 4;
    static constexpr size_t SR_ELEMENT_SIZE = 4;
    static constexpr size_t SR_ALIGN = 16;

    static constexpr size_t CR_COUNT = 1;
    static constexpr size_t CR_ELEMENTS = 4;
    static constexpr size_t CR_ELEMENT_SIZE = 4;
    static constexpr size_t CR_ALIGN = 16;

    static constexpr size_t IP_COUNT = 1;
    static constexpr size_t IP_ELEMENTS = 1;
    static constexpr size_t IP_ELEMENT_SIZE = 4;
    static constexpr size_t IP_ALIGN = 28;

    static constexpr size_t N_COUNT = 1;
    static constexpr size_t N_ELEMENTS = 2;
    static constexpr size_t N_ELEMENT_SIZE = 4;
    static constexpr size_t N_ALIGN = 24;

    static constexpr size_t TDR_COUNT = 0;
    static constexpr size_t TDR_ELEMENTS = 0;
    static constexpr size_t TDR_ELEMENT_SIZE = 0;
    static constexpr size_t TDR_ALIGN = 0;

    static constexpr size_t ACC_COUNT = 4;
    static constexpr size_t ACC_ELEMENTS = 8;
    static constexpr size_t ACC_ELEMENT_SIZE = 4;
    static constexpr size_t ACC_ALIGN = 0;

    static constexpr size_t MME_COUNT = 8;
    static constexpr size_t MME_ELEMENTS = 8;
    static constexpr size_t MME_ELEMENT_SIZE = 4;
    static constexpr size_t MME_ALIGN = 0;

    static constexpr size_t TM_COUNT = 1;
    static constexpr size_t TM_ELEMENTS = 5;
    static constexpr size_t TM_ELEMENT_SIZE = 4;
    static constexpr size_t TM_ALIGN = 12;

    static constexpr size_t CE_COUNT = 1;
    static constexpr size_t CE_ELEMENTS = 1;
    static constexpr size_t CE_ELEMENT_SIZE = 4;
    static constexpr size_t CE_ALIGN = 28;

    static constexpr size_t SP_COUNT = 0;
    static constexpr size_t SP_ELEMENTS = 0;
    static constexpr size_t SP_ELEMENT_SIZE = 0;
    static constexpr size_t SP_ALIGN = 32;

    static constexpr size_t DBG_COUNT = 1;
    static constexpr size_t DBG_ELEMENTS = 1;
    static constexpr size_t DBG_ELEMENT_SIZE = 4;
    static constexpr size_t DBG_ALIGN = 0;

    static constexpr size_t VERSION_COUNT = 1;
    static constexpr size_t VERSION_ELEMENTS = 1;
    static constexpr size_t VERSION_ELEMENT_SIZE = 20;
    static constexpr size_t VERSION_ALIGN = 8;

    static constexpr size_t SIP_CMD_COUNT = 1;
    static constexpr size_t SIP_CMD_ELEMENTS = 1;
    static constexpr size_t SIP_CMD_ELEMENT_SIZE = 128;
    static constexpr size_t SIP_CMD_ALIGN = 0;

    static constexpr size_t CONTEXT_ID_COUNT = 1;
    static constexpr size_t CONTEXT_ID_ELEMENTS = 1;
    static constexpr size_t CONTEXT_ID_ELEMENT_SIZE = 8;
    static constexpr size_t CONTEXT_ID_ALIGN = 24;

    static constexpr size_t DBG_REG_COUNT = 1;
    static constexpr size_t DBG_REG_ELEMENTS = 2;
    static constexpr size_t DBG_REG_ELEMENT_SIZE = 4;
    static constexpr size_t DBG_REG_ALIGN = 24;

    uint8_t grf[GR_COUNT * GR_ELEMENTS * GR_ELEMENT_SIZE + GR_ALIGN];
    uint8_t a0[A0_COUNT * A0_ELEMENTS * A0_ELEMENT_SIZE + A0_ALIGN];
    uint8_t f[F_COUNT * F_ELEMENTS * F_ELEMENT_SIZE + F_ALIGN];
    uint8_t execmask[EXEC_MASK_COUNT * EXEC_MASK_ELEMENTS * EXEC_MASK_ELEMENT_SIZE + EXEC_MASK_ALIGN];
    uint8_t sr[SR_COUNT * SR_ELEMENTS * SR_ELEMENT_SIZE + SR_ALIGN];
    uint8_t cr[CR_COUNT * CR_ELEMENTS * CR_ELEMENT_SIZE + CR_ALIGN];
    uint8_t n[N_COUNT * N_ELEMENTS * N_ELEMENT_SIZE + N_ALIGN];
    uint8_t acc[ACC_COUNT * ACC_ELEMENTS * ACC_ELEMENT_SIZE + ACC_ALIGN];
    uint8_t mme[MME_COUNT * MME_ELEMENTS * MME_ELEMENT_SIZE + MME_ALIGN];
    uint8_t tm[TM_COUNT * TM_ELEMENTS * TM_ELEMENT_SIZE + TM_ALIGN];
    uint8_t ce[CE_COUNT * CE_ELEMENTS * CE_ELEMENT_SIZE + CE_ALIGN];
    uint8_t sp[SP_COUNT * SP_ELEMENTS * SP_ELEMENT_SIZE + SP_ALIGN];
    uint8_t dbg[DBG_COUNT * DBG_ELEMENTS * DBG_ELEMENT_SIZE + DBG_ALIGN];
    uint8_t version[VERSION_COUNT * VERSION_ELEMENTS * VERSION_ELEMENT_SIZE + VERSION_ALIGN];
    uint8_t sip_cmd[SIP_CMD_COUNT * SIP_CMD_ELEMENTS * SIP_CMD_ELEMENT_SIZE + SIP_CMD_ALIGN];
    uint8_t ctx[CONTEXT_ID_COUNT * CONTEXT_ID_ELEMENTS *CONTEXT_ID_ELEMENT_SIZE +CONTEXT_ID_ALIGN];
    uint8_t dbg_reg[DBG_REG_COUNT * DBG_REG_ELEMENTS * DBG_REG_ELEMENT_SIZE +DBG_REG_ALIGN];
};

static struct StateSaveAreaHeader XeHPGSIPCSRDebugBindlessDebugHeader =
{
    {"tssarea", 0, {2, 2, 0}, sizeof(StateSaveAreaHeader) / 8, {0, 0, 0}},  // versionHeader
    {
        // regHeader
        0,                               // num_slices
        0,                               // num_subslices_per_slice
        0,                               // num_eus_per_subslice
        0,                               // num_threads_per_eu
        0,                               // state_area_offset
        0x2800,                          // state_save_size
        0,                               // slm_area_offset
        0,                               // slm_bank_size
        0,                               // slm_bank_valid
        offsetof(struct XeHPGDebugSurfaceLayout, version),  // sr_magic_offset
        {offsetof(struct XeHPGDebugSurfaceLayout, grf), XeHPGDebugSurfaceLayout::GR_COUNT,
         XeHPGDebugSurfaceLayout::GR_ELEMENTS* XeHPGDebugSurfaceLayout::GR_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::GR_ELEMENTS* XeHPGDebugSurfaceLayout::GR_ELEMENT_SIZE},  // grf
        {offsetof(struct XeHPGDebugSurfaceLayout, a0), XeHPGDebugSurfaceLayout::A0_COUNT,
         XeHPGDebugSurfaceLayout::A0_ELEMENTS* XeHPGDebugSurfaceLayout::A0_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::A0_ELEMENTS* XeHPGDebugSurfaceLayout::A0_ELEMENT_SIZE},  // addr
        {offsetof(struct XeHPGDebugSurfaceLayout, f), XeHPGDebugSurfaceLayout::F_COUNT,
         XeHPGDebugSurfaceLayout::F_ELEMENTS* XeHPGDebugSurfaceLayout::F_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::F_ELEMENTS* XeHPGDebugSurfaceLayout::F_ELEMENT_SIZE},  // flag
        {offsetof(struct XeHPGDebugSurfaceLayout, execmask), XeHPGDebugSurfaceLayout::EXEC_MASK_COUNT,
         XeHPGDebugSurfaceLayout::EXEC_MASK_ELEMENTS* XeHPGDebugSurfaceLayout::EXEC_MASK_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::EXEC_MASK_ELEMENTS* XeHPGDebugSurfaceLayout::EXEC_MASK_ELEMENT_SIZE},  // emask
        {offsetof(struct XeHPGDebugSurfaceLayout, sr), XeHPGDebugSurfaceLayout::SR_COUNT,
         XeHPGDebugSurfaceLayout::SR_ELEMENTS* XeHPGDebugSurfaceLayout::SR_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::SR_ELEMENTS* XeHPGDebugSurfaceLayout::SR_ELEMENT_SIZE},  // sr
        {offsetof(struct XeHPGDebugSurfaceLayout, cr), XeHPGDebugSurfaceLayout::CR_COUNT,
         XeHPGDebugSurfaceLayout::CR_ELEMENTS* XeHPGDebugSurfaceLayout::CR_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::CR_ELEMENTS* XeHPGDebugSurfaceLayout::CR_ELEMENT_SIZE},  // cr
        {offsetof(struct XeHPGDebugSurfaceLayout, n), XeHPGDebugSurfaceLayout::N_COUNT,
         XeHPGDebugSurfaceLayout::N_ELEMENTS* XeHPGDebugSurfaceLayout::N_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::N_ELEMENTS* XeHPGDebugSurfaceLayout::N_ELEMENT_SIZE},  // notification
        {0, 0, 0, 0},                                                                   // tdr
        {offsetof(struct XeHPGDebugSurfaceLayout, acc), XeHPGDebugSurfaceLayout::ACC_COUNT,
         XeHPGDebugSurfaceLayout::ACC_ELEMENTS* XeHPGDebugSurfaceLayout::ACC_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::ACC_ELEMENTS* XeHPGDebugSurfaceLayout::ACC_ELEMENT_SIZE},  // acc
        {offsetof(struct XeHPGDebugSurfaceLayout, mme), XeHPGDebugSurfaceLayout::MME_COUNT,
         XeHPGDebugSurfaceLayout::MME_ELEMENTS* XeHPGDebugSurfaceLayout::MME_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::MME_ELEMENTS* XeHPGDebugSurfaceLayout::MME_ELEMENT_SIZE}, // mme
        {offsetof(struct XeHPGDebugSurfaceLayout, ce), XeHPGDebugSurfaceLayout::CE_COUNT,
         XeHPGDebugSurfaceLayout::CE_ELEMENTS* XeHPGDebugSurfaceLayout::CE_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::CE_ELEMENTS* XeHPGDebugSurfaceLayout::CE_ELEMENT_SIZE},  // ce
        {offsetof(struct XeHPGDebugSurfaceLayout, sp), XeHPGDebugSurfaceLayout::SP_COUNT,
         XeHPGDebugSurfaceLayout::SP_ELEMENTS* XeHPGDebugSurfaceLayout::SP_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::SP_ELEMENTS* XeHPGDebugSurfaceLayout::SP_ELEMENT_SIZE},  // sp
        {offsetof(struct XeHPGDebugSurfaceLayout, sip_cmd), XeHPGDebugSurfaceLayout::SIP_CMD_COUNT,
         XeHPGDebugSurfaceLayout::SIP_CMD_ELEMENTS* XeHPGDebugSurfaceLayout::SIP_CMD_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::SIP_CMD_ELEMENTS* XeHPGDebugSurfaceLayout::SIP_CMD_ELEMENT_SIZE},  // cmd
        {offsetof(struct XeHPGDebugSurfaceLayout, tm), XeHPGDebugSurfaceLayout::TM_COUNT,
         XeHPGDebugSurfaceLayout::TM_ELEMENTS* XeHPGDebugSurfaceLayout::TM_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::TM_ELEMENTS* XeHPGDebugSurfaceLayout::TM_ELEMENT_SIZE},  // tm
        {0, 0, 0, 0},                                  // FC
        {offsetof(struct XeHPGDebugSurfaceLayout, dbg), XeHPGDebugSurfaceLayout::DBG_COUNT,
         XeHPGDebugSurfaceLayout::DBG_ELEMENTS* XeHPGDebugSurfaceLayout::DBG_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::DBG_ELEMENTS* XeHPGDebugSurfaceLayout::DBG_ELEMENT_SIZE},  // dbg
        {offsetof(struct XeHPGDebugSurfaceLayout, ctx), XeHPGDebugSurfaceLayout::CONTEXT_ID_COUNT,
         XeHPGDebugSurfaceLayout::CONTEXT_ID_ELEMENTS* XeHPGDebugSurfaceLayout::CONTEXT_ID_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::CONTEXT_ID_ELEMENTS* XeHPGDebugSurfaceLayout::CONTEXT_ID_ELEMENT_SIZE}, // context id
        {offsetof(struct XeHPGDebugSurfaceLayout, dbg_reg), XeHPGDebugSurfaceLayout::DBG_REG_COUNT,
         XeHPGDebugSurfaceLayout::DBG_REG_ELEMENTS* XeHPGDebugSurfaceLayout::DBG_REG_ELEMENT_SIZE * 8,
         XeHPGDebugSurfaceLayout::DBG_REG_ELEMENTS* XeHPGDebugSurfaceLayout::DBG_REG_ELEMENT_SIZE}, // dbg registers
    }
};

bool SIPSuppoertedOnPlatformFamily(const GFXCORE_FAMILY& family)
{
    switch (family)
    {
    case IGFX_GEN9_CORE:
    case IGFX_GENNEXT_CORE:
    case IGFX_GEN10_CORE:
    case IGFX_GEN11_CORE:
    case IGFX_GEN12_CORE:
    case IGFX_GEN12LP_CORE:
    case IGFX_XE_HP_CORE:
    case IGFX_XE_HPG_CORE:
    case IGFX_XE_HPC_CORE:
    case IGFX_XE2_HPG_CORE:
    case IGFX_XE3_CORE:
        return true;
    default:
        return false;
    }
}

MemoryBuffer* LoadFile(const std::string &FileName)
{
    ErrorOr<std::unique_ptr<MemoryBuffer>> result = MemoryBuffer::getFile(FileName.c_str());
    return result.get().release();
}

bool CSystemThread::CreateSystemThreadKernel(
    const IGC::CPlatform &platform,
    const SYSTEM_THREAD_MODE mode,
    USC::SSystemThreadKernelOutput* &pSystemThreadKernelOutput,
    bool bindlessMode)
{
    if (!SIPSuppoertedOnPlatformFamily(platform.getPlatformInfo().eRenderCoreFamily))
    {
        return false;
    }

    bool success = true;

    // Check if the System Thread mode in the correct range.
    if( !( ( mode & SYSTEM_THREAD_MODE_DEBUG ) ||
           ( mode & SYSTEM_THREAD_MODE_DEBUG_LOCAL ) ||
           ( mode & SYSTEM_THREAD_MODE_CSR ) ) )
    {
        success = false;
        IGC_ASSERT(success);
    }

    // Create System Thread kernel program.
    if( success )
    {
        CGenSystemInstructionKernelProgram* pKernelProgram =
            new CGenSystemInstructionKernelProgram(mode);
        success = pKernelProgram ? true : false;
        IGC_ASSERT(success);

        // Allocate memory for SSystemThreadKernelOutput.
        if( success )
        {
            IGC_ASSERT(pSystemThreadKernelOutput == nullptr);
            pSystemThreadKernelOutput = new SSystemThreadKernelOutput;

            success = ( pSystemThreadKernelOutput != nullptr );
            IGC_ASSERT(success);

            if( success )
            {
                memset(
                    pSystemThreadKernelOutput,
                    0 ,
                    sizeof( SSystemThreadKernelOutput ) );
            }
        }

        const unsigned int DQWORD_SIZE = 2 * sizeof( unsigned long long );

        if( pKernelProgram )
        {
            pKernelProgram->Create( platform, mode, bindlessMode);

            pSystemThreadKernelOutput->m_KernelProgramSize = pKernelProgram->GetProgramSize();
            pSystemThreadKernelOutput->m_StateSaveAreaHeaderSize = pKernelProgram->GetStateSaveHeaderSize();

            const IGC::SCompilerHwCaps& Caps = const_cast<IGC::CPlatform&>( platform ).GetCaps();

            if( mode & SYSTEM_THREAD_MODE_CSR )
                pSystemThreadKernelOutput->m_SystemThreadScratchSpace = Caps.KernelHwCaps.CsrSizeInMb * sizeof( MEGABYTE );

            if( mode & ( SYSTEM_THREAD_MODE_DEBUG | SYSTEM_THREAD_MODE_DEBUG_LOCAL ) )
                pSystemThreadKernelOutput->m_SystemThreadResourceSize = Caps.KernelHwCaps.CsrSizeInMb * 2 * sizeof( MEGABYTE );

            pSystemThreadKernelOutput->m_pKernelProgram =
                IGC::aligned_malloc( pSystemThreadKernelOutput->m_KernelProgramSize, DQWORD_SIZE );

        if (pSystemThreadKernelOutput->m_StateSaveAreaHeaderSize)
                pSystemThreadKernelOutput->m_pStateSaveAreaHeader =
                    IGC::aligned_malloc( pSystemThreadKernelOutput->m_StateSaveAreaHeaderSize, DQWORD_SIZE );

            success = ( pSystemThreadKernelOutput->m_pKernelProgram != nullptr );
            if (pSystemThreadKernelOutput->m_StateSaveAreaHeaderSize)
                success &= ( pSystemThreadKernelOutput->m_pStateSaveAreaHeader != nullptr );
        }

        if( success )
        {
            const void *pStartAddress = pKernelProgram->GetLinearAddress();
            const void *pStateSaveAddress = pKernelProgram->GetStateSaveHeaderAddress();

            if( !pStartAddress ||
          (pSystemThreadKernelOutput->m_StateSaveAreaHeaderSize && !pStateSaveAddress))
            {
                IGC_ASSERT(0);
                success = false;
            }

            if( success )
            {
                memcpy_s(
                    pSystemThreadKernelOutput->m_pKernelProgram,
                    pSystemThreadKernelOutput->m_KernelProgramSize,
                    pStartAddress,
                    pSystemThreadKernelOutput->m_KernelProgramSize );

                if (pSystemThreadKernelOutput->m_StateSaveAreaHeaderSize)
                    memcpy_s(
                        pSystemThreadKernelOutput->m_pStateSaveAreaHeader,
                        pSystemThreadKernelOutput->m_StateSaveAreaHeaderSize,
                        pStateSaveAddress,
                        pSystemThreadKernelOutput->m_StateSaveAreaHeaderSize);
            }
        }
        else
        {
            IGC_ASSERT(0);
            success = false;
        }

        if( pKernelProgram )
        {
            pKernelProgram->Delete( pKernelProgram );
            delete pKernelProgram;
        }
    }
    if (!success)
    {
        if (pSystemThreadKernelOutput)
        {
            IGC::aligned_free(pSystemThreadKernelOutput->m_pKernelProgram);
            pSystemThreadKernelOutput->m_pKernelProgram = nullptr;
            IGC::aligned_free(pSystemThreadKernelOutput->m_pStateSaveAreaHeader);
            pSystemThreadKernelOutput->m_pStateSaveAreaHeader = nullptr;
            delete pSystemThreadKernelOutput;
            pSystemThreadKernelOutput = nullptr;
        }
    }
    return success;
}

void CSystemThread::DeleteSystemThreadKernel(
    USC::SSystemThreadKernelOutput* &pSystemThreadKernelOutput )
{
    IGC::aligned_free(pSystemThreadKernelOutput->m_pKernelProgram);
    if (pSystemThreadKernelOutput->m_pStateSaveAreaHeader)
        IGC::aligned_free(pSystemThreadKernelOutput->m_pStateSaveAreaHeader);
    delete pSystemThreadKernelOutput;
    pSystemThreadKernelOutput = nullptr;
}

//populate the SIPKernelInfo map with starting address and size of every SIP kernels
void populateSIPKernelInfo(const IGC::CPlatform &platform,
        std::map< unsigned char, std::tuple<void*, unsigned int, void*, unsigned int> > &SIPKernelInfo)
{
    //LLVM_UPGRADE_TODO
    // check if (int)sizeof(T) is ok or change the pair def for SIPKernelInfo
    SIPKernelInfo[GEN9_SIP_DEBUG] = std::make_tuple((void*)&Gen9SIPDebug, (int)sizeof(Gen9SIPDebug), nullptr, 0);

    SIPKernelInfo[GEN9_SIP_CSR] = std::make_tuple((void*)&Gen9SIPCSR, (int)sizeof(Gen9SIPCSR), nullptr, 0);

    SIPKernelInfo[GEN9_SIP_CSR_DEBUG] = std::make_tuple((void*)&Gen9SIPCSRDebug, (int)sizeof(Gen9SIPCSRDebug), nullptr, 0);

    SIPKernelInfo[GEN10_SIP_DEBUG] = std::make_tuple((void*)&Gen10SIPDebug, (int)sizeof(Gen10SIPDebug), nullptr, 0);

    SIPKernelInfo[GEN10_SIP_CSR] = std::make_tuple((void*)&Gen10SIPCSR, (int)sizeof(Gen10SIPCSR), nullptr, 0);

    SIPKernelInfo[GEN10_SIP_CSR_DEBUG] = std::make_tuple((void*)&Gen10SIPCSRDebug, (int)sizeof(Gen10SIPCSRDebug), nullptr, 0);

    SIPKernelInfo[GEN9_SIP_DEBUG_BINDLESS] = std::make_tuple((void*)&Gen9SIPDebugBindless, (int)sizeof(Gen9SIPDebugBindless), nullptr, 0);

    SIPKernelInfo[GEN10_SIP_DEBUG_BINDLESS] = std::make_tuple((void*)&Gen10SIPDebugBindless, (int)sizeof(Gen10SIPDebugBindless), nullptr, 0);

    SIPKernelInfo[GEN9_BXT_SIP_CSR] = std::make_tuple((void*)&Gen9BXTSIPCSR, (int)sizeof(Gen9BXTSIPCSR), nullptr, 0);

    SIPKernelInfo[GEN9_SIP_CSR_DEBUG_LOCAL] = std::make_tuple((void*)&Gen9SIPCSRDebugLocal, (int)sizeof(Gen9SIPCSRDebugLocal), nullptr, 0);

    SIPKernelInfo[GEN9_GLV_SIP_CSR] = std::make_tuple((void*)&Gen9GLVSIPCSR, (int)sizeof(Gen9GLVSIPCSR), nullptr, 0);

    SIPKernelInfo[GEN11_SIP_CSR] = std::make_tuple((void*)&Gen11SIPCSR, (int)sizeof(Gen11SIPCSR), nullptr, 0);

    SIPKernelInfo[GEN11_SIP_CSR_DEBUG] = std::make_tuple((void*)&Gen11SIPCSRDebug, (int)sizeof(Gen11SIPCSRDebug), nullptr, 0);

    SIPKernelInfo[GEN11_SIP_CSR_DEBUG_BINDLESS] = std::make_tuple((void*)&Gen11SIPCSRDebugBindless, (int)sizeof(Gen11SIPCSRDebugBindless), nullptr, 0);

    SIPKernelInfo[GEN11_LKF_SIP_CSR] = std::make_tuple((void*)&Gen11LKFSIPCSR, (int)sizeof(Gen11LKFSIPCSR), nullptr, 0);

    SIPKernelInfo[GEN12_LP_CSR] = std::make_tuple((void*)&Gen12LPSIPCSR, (int)sizeof(Gen12LPSIPCSR), nullptr, 0);

    SIPKernelInfo[GEN12_LP_CSR_DEBUG] = std::make_tuple((void*)&Gen12LPSIPCSRDebug, (int)sizeof(Gen12LPSIPCSRDebug), nullptr, 0);

    SIPKernelInfo[GEN12_LP_CSR_DEBUG_BINDLESS] = std::make_tuple((void*)&Gen12LPSIPCSRDebugBindless,
            (int)sizeof(Gen12LPSIPCSRDebugBindless),
            (void*)&Gen12SIPCSRDebugBindlessDebugHeader,
            (int)Gen12SIPCSRDebugBindlessDebugHeader.versionHeader.size * 8);

    SIPKernelInfo[XE_HP_CSR_DEBUG_BINDLESS] = std::make_tuple((void*)&XeHPSIPCSRDebugBindless,
        (int)sizeof(XeHPSIPCSRDebugBindless),
        (void*)&Gen12SIPCSRDebugBindlessDebugHeader,
        (int)sizeof(Gen12SIPCSRDebugBindlessDebugHeader));

    SIPKernelInfo[XE_HPG_CSR_DEBUG_BINDLESS] = std::make_tuple((void*)&XeHPGSIPCSRDebugBindless,
            (int)sizeof(XeHPGSIPCSRDebugBindless),
            (void*)&XeHPGSIPCSRDebugBindlessDebugHeader,
            (int)sizeof(XeHPGSIPCSRDebugBindlessDebugHeader));

    GT_SYSTEM_INFO sysInfo = platform.GetGTSystemInfo();
    Gen12SIPCSRDebugBindlessDebugHeader.regHeader.num_slices = sysInfo.MaxSlicesSupported;
    IGC_ASSERT(sysInfo.MaxSlicesSupported > 0);
    Gen12SIPCSRDebugBindlessDebugHeader.regHeader.num_subslices_per_slice =
            (sysInfo.MaxSlicesSupported > 0 ? (sysInfo.MaxSubSlicesSupported / sysInfo.MaxSlicesSupported) : sysInfo.MaxSubSlicesSupported);
    Gen12SIPCSRDebugBindlessDebugHeader.regHeader.num_eus_per_subslice = sysInfo.MaxEuPerSubSlice;
    Gen12SIPCSRDebugBindlessDebugHeader.regHeader.num_threads_per_eu = 0;

    // Avoid division by zero error in case if any of the sysInfo parameter is zero.
    if (sysInfo.EUCount != 0)
        Gen12SIPCSRDebugBindlessDebugHeader.regHeader.num_threads_per_eu = (sysInfo.ThreadCount / sysInfo.EUCount);

    if (sizeof(StateSaveAreaHeader) % 16)
        Gen12SIPCSRDebugBindlessDebugHeader.regHeader.state_area_offset =
            16 - sizeof(StateSaveAreaHeader) % 16;

    // Match SIP alignment of debug surface
    IGC_ASSERT_MESSAGE(((Gen12SIPCSRDebugBindlessDebugHeader.regHeader.state_area_offset +
        Gen12SIPCSRDebugBindlessDebugHeader.versionHeader.size * 8) / 16 == 0x14),
        "Gen12 Bindless SIP header size alignment mismatch.");

    SIPKernelInfo[XE_HP_CSR] = std::make_tuple((void*)&XeHPSIPCSR, (int)sizeof(XeHPSIPCSR), nullptr, 0);
    SIPKernelInfo[XE_HP_CSR_DEBUG] = std::make_tuple((void*)&XeHPSIPCSRDebug, (int)sizeof(XeHPSIPCSRDebug), nullptr, 0);

    SIPKernelInfo[XE_HPG_CSR_DEBUG] = std::make_tuple((void*)&XeHPGSIPCSRDebug, (int)sizeof(XeHPGSIPCSRDebug), nullptr, 0);

    SIPKernelInfo[XE_HPC_CSR_DEBUG_BINDLESS] = std::make_tuple((void*)&XeHPCSIPCSRDebugBindless,
              (int)sizeof(XeHPCSIPCSRDebugBindless), (void*)&XeHPCSIPCSRDebugBindlessDebugHeader,
              (int)sizeof(XeHPCSIPCSRDebugBindlessDebugHeader));

    XeHPCSIPCSRDebugBindlessDebugHeader.regHeader.num_threads_per_eu = 0;

    if (sysInfo.EUCount != 0)
        XeHPCSIPCSRDebugBindlessDebugHeader.regHeader.num_threads_per_eu = (sysInfo.ThreadCount / sysInfo.EUCount);

    if (sizeof(StateSaveAreaHeader) % 16)
        XeHPCSIPCSRDebugBindlessDebugHeader.regHeader.state_area_offset =
            16 - sizeof(StateSaveAreaHeader) % 16;

    XeHPCSIPCSRDebugBindlessDebugHeader.regHeader.num_slices = sysInfo.MaxSlicesSupported;
    XeHPCSIPCSRDebugBindlessDebugHeader.regHeader.num_subslices_per_slice =
            (sysInfo.MaxSlicesSupported > 0 ? (sysInfo.MaxSubSlicesSupported / sysInfo.MaxSlicesSupported) : sysInfo.MaxSubSlicesSupported);
    XeHPCSIPCSRDebugBindlessDebugHeader.regHeader.num_eus_per_subslice = sysInfo.MaxEuPerSubSlice;

    XeHPGSIPCSRDebugBindlessDebugHeader.regHeader.num_threads_per_eu = 0;

    if (sysInfo.EUCount != 0)
        XeHPGSIPCSRDebugBindlessDebugHeader.regHeader.num_threads_per_eu = (sysInfo.ThreadCount / sysInfo.EUCount);

    if (sizeof(StateSaveAreaHeader) % 16)
        XeHPGSIPCSRDebugBindlessDebugHeader.regHeader.state_area_offset =
            16 - sizeof(StateSaveAreaHeader) % 16;

    XeHPGSIPCSRDebugBindlessDebugHeader.regHeader.num_slices = sysInfo.MaxSlicesSupported;
    XeHPGSIPCSRDebugBindlessDebugHeader.regHeader.num_subslices_per_slice =
            (sysInfo.MaxSlicesSupported > 0 ? (sysInfo.MaxSubSlicesSupported / sysInfo.MaxSlicesSupported) : sysInfo.MaxSubSlicesSupported);
    XeHPGSIPCSRDebugBindlessDebugHeader.regHeader.num_eus_per_subslice = sysInfo.MaxEuPerSubSlice;

    // Xe2
     SIPKernelInfo[XE2_CSR_DEBUG_BINDLESS] = std::make_tuple((void*)&Xe2SIPCSRDebugBindless,
              (int)sizeof(Xe2SIPCSRDebugBindless), (void*)&Xe2SIPCSRDebugBindlessDebugHeader,
              (int)sizeof(Xe2SIPCSRDebugBindlessDebugHeader));

    Xe2SIPCSRDebugBindlessDebugHeader.regHeader.num_threads_per_eu = 0;

    if (sysInfo.EUCount != 0)
        Xe2SIPCSRDebugBindlessDebugHeader.regHeader.num_threads_per_eu = (sysInfo.ThreadCount / sysInfo.EUCount);

    if (sizeof(StateSaveAreaHeader) % 16)
        Xe2SIPCSRDebugBindlessDebugHeader.regHeader.state_area_offset =
            16 - sizeof(StateSaveAreaHeader) % 16;

    Xe2SIPCSRDebugBindlessDebugHeader.regHeader.num_slices = sysInfo.MaxSlicesSupported;
    Xe2SIPCSRDebugBindlessDebugHeader.regHeader.num_subslices_per_slice =
            (sysInfo.MaxSlicesSupported > 0 ? (sysInfo.MaxSubSlicesSupported / sysInfo.MaxSlicesSupported) : sysInfo.MaxSubSlicesSupported);
    Xe2SIPCSRDebugBindlessDebugHeader.regHeader.num_eus_per_subslice = sysInfo.MaxEuPerSubSlice;

    // wmtp Xe2 SIP
    {
        // Xe2 128
        SIPKernelInfo[XE2_CSR_DEBUG_BINDLESS_config128] = std::make_tuple(
            (void *)&XE2_config_128, (int)sizeof(XE2_config_128),
            (void *)&Xe2SIP_WMTP_CSRDebugBindlessDebugHeader,  (int)sizeof(Xe2SIP_WMTP_CSRDebugBindlessDebugHeader));

        // Xe2 160
        SIPKernelInfo[XE2_CSR_DEBUG_BINDLESS_config160] = std::make_tuple(
            (void *)&XE2_config_160, (int)sizeof(XE2_config_160),
            (void *)&Xe2SIP_WMTP_CSRDebugBindlessDebugHeader,  (int)sizeof(Xe2SIP_WMTP_CSRDebugBindlessDebugHeader));

     }

    // PTL wmtp Sip
    {
        // Xe3 1x4
        SIPKernelInfo[XE3_CSR_DEBUG_BINDLESS_config_1x4] = std::make_tuple(
                (void *)&Xe3_PTL_config_1x4, (int)sizeof(Xe3_PTL_config_1x4),
                (void *)&Xe3SIP_config_1x4_WMTP_CSRDebugBindlessDebugHeader,  (int)sizeof(Xe3SIP_config_1x4_WMTP_CSRDebugBindlessDebugHeader));

        // Xe3 2x6
        SIPKernelInfo[XE3_CSR_DEBUG_BINDLESS_config_2x6] = std::make_tuple(
                (void *)&Xe3_PTL_config_2x6, (int)sizeof(Xe3_PTL_config_2x6),
                (void *)&Xe3SIP_config_2x6_WMTP_CSRDebugBindlessDebugHeader,  (int)sizeof(Xe3SIP_config_2x6_WMTP_CSRDebugBindlessDebugHeader));
    }
    
    // Xe3 / Xe3G
    SIPKernelInfo[XE3G_DEBUG_BINDLESS] = std::make_tuple((void*)&Xe3_G_SIPDebugBindless,
              (int)sizeof(Xe3_G_SIPDebugBindless), (void*)&Xe3_G_SIPDebugBindlessDebugHeaderV3,
              (int)sizeof(Xe3_G_SIPDebugBindlessDebugHeaderV3));

    Xe3_G_SIPDebugBindlessDebugHeaderV3.regHeader.num_threads_per_eu = 0;

    if (sysInfo.EUCount != 0)
        Xe3_G_SIPDebugBindlessDebugHeaderV3.regHeader.num_threads_per_eu = (sysInfo.ThreadCount / sysInfo.EUCount);

    if (sizeof(StateSaveAreaHeader) % 16)
        Xe3_G_SIPDebugBindlessDebugHeaderV3.regHeader.state_area_offset =
            16 - sizeof(StateSaveAreaHeader) % 16;

    Xe3_G_SIPDebugBindlessDebugHeaderV3.regHeader.num_slices = sysInfo.MaxSlicesSupported;
    Xe3_G_SIPDebugBindlessDebugHeaderV3.regHeader.num_subslices_per_slice =
            (sysInfo.MaxSlicesSupported > 0 ? (sysInfo.MaxSubSlicesSupported / sysInfo.MaxSlicesSupported) : sysInfo.MaxSubSlicesSupported);
    Xe3_G_SIPDebugBindlessDebugHeaderV3.regHeader.num_eus_per_subslice = sysInfo.MaxEuPerSubSlice;

}

CGenSystemInstructionKernelProgram* CGenSystemInstructionKernelProgram::Create(
    const IGC::CPlatform &platform,
    const SYSTEM_THREAD_MODE mode,
    const bool bindlessMode)
{
    unsigned char SIPIndex = 0;
    std::map< unsigned char, std::tuple<void*, unsigned int, void*, unsigned int> > SIPKernelInfo;
    populateSIPKernelInfo(platform, SIPKernelInfo);

    GT_SYSTEM_INFO sysInfo = platform.GetGTSystemInfo();

    switch( platform.getPlatformInfo().eRenderCoreFamily )
    {
    case IGFX_GEN9_CORE:
    case IGFX_GENNEXT_CORE:
    {
        if (mode & SYSTEM_THREAD_MODE_DEBUG)
        {
            SIPIndex = bindlessMode ? GEN9_SIP_DEBUG_BINDLESS : GEN9_SIP_DEBUG;
        }
        else if (bindlessMode)
        {
            //Add the rest later for bindless mode for preemption
        }
        else if (mode == (SYSTEM_THREAD_MODE_CSR | SYSTEM_THREAD_MODE_DEBUG_LOCAL))
        {
            SIPIndex = GEN9_SIP_CSR_DEBUG_LOCAL;
        }
        else if (mode == SYSTEM_THREAD_MODE_CSR)
        {
            switch (platform.getPlatformInfo().eProductFamily)
            {
           case IGFX_BROXTON:
           case IGFX_GEMINILAKE:
               /*Special SIP for 2x6 from HW team with 64KB offset*/
               SIPIndex = GEN9_BXT_SIP_CSR;
               break;

            default:
               SIPIndex = GEN9_SIP_CSR;
               break;
            }
        }
        break;
    }
    case IGFX_GEN10_CORE:
    {
        if (mode & SYSTEM_THREAD_MODE_DEBUG)
        {
            SIPIndex = bindlessMode ? GEN10_SIP_DEBUG_BINDLESS : GEN10_SIP_DEBUG;
        }
        else if (bindlessMode)
        {
            //Add the rest later for bindless mode for preemption
        }
        else if (mode == SYSTEM_THREAD_MODE_CSR)
        {
            SIPIndex = GEN10_SIP_CSR;
        }
        break;
    }
    case IGFX_GEN11_CORE:
    {
        if (mode & SYSTEM_THREAD_MODE_DEBUG)
        {
            SIPIndex = bindlessMode ? GEN11_SIP_CSR_DEBUG_BINDLESS : GEN11_SIP_CSR_DEBUG;
        }
        else if (bindlessMode)
        {
            //Add the rest later for bindless mode for preemption
        }
        else if (mode == SYSTEM_THREAD_MODE_CSR)
        {
            switch (platform.getPlatformInfo().eProductFamily)
            {
            case IGFX_ICELAKE_LP:
                SIPIndex = GEN11_SIP_CSR;
                break;

            case IGFX_LAKEFIELD:
             case IGFX_ELKHARTLAKE: // same as IGFX_JASPERLAKE
                SIPIndex = GEN11_LKF_SIP_CSR;
                break;

            default:
                break;
            }
        }
        break;
    }
    case IGFX_GEN12_CORE:
    case IGFX_GEN12LP_CORE:
    case IGFX_XE_HP_CORE:
    case IGFX_XE_HPG_CORE:
    case IGFX_XE_HPC_CORE:
    case IGFX_XE2_HPG_CORE:

    {
        if (mode & SYSTEM_THREAD_MODE_DEBUG)
        {
            switch (platform.getPlatformInfo().eProductFamily)
            {
            case IGFX_TIGERLAKE_LP:
            case IGFX_DG1:
            case IGFX_ROCKETLAKE:
            case IGFX_ALDERLAKE_S:
            case IGFX_ALDERLAKE_P:
            case IGFX_ALDERLAKE_N:
                SIPIndex = bindlessMode ? GEN12_LP_CSR_DEBUG_BINDLESS : GEN12_LP_CSR_DEBUG;
                break;
            case IGFX_XE_HP_SDV:
                SIPIndex = bindlessMode ? XE_HP_CSR_DEBUG_BINDLESS : XE_HP_CSR_DEBUG;
                break;
            case IGFX_DG2:
            case IGFX_METEORLAKE:
            case IGFX_ARROWLAKE:
                SIPIndex =  bindlessMode ? XE_HPG_CSR_DEBUG_BINDLESS : XE_HPG_CSR_DEBUG;
                break;
      // No support for Bindful mode for PVC.
            case IGFX_PVC:
                SIPIndex =  XE_HPC_CSR_DEBUG_BINDLESS;
                break;
            case IGFX_LUNARLAKE :
            case IGFX_BMG :
                SIPIndex =  XE2_CSR_DEBUG_BINDLESS;
            default:
                break;
            }
        }
        else if (bindlessMode)
        {
            //Add the rest later for bindless mode for preemption
        }
        else if (mode == SYSTEM_THREAD_MODE_CSR)
        {
            switch (platform.getPlatformInfo().eProductFamily)
            {
            case IGFX_TIGERLAKE_LP:
            case IGFX_DG1:
            case IGFX_ROCKETLAKE:
            case IGFX_ALDERLAKE_S:
            case IGFX_ALDERLAKE_P:
            case IGFX_ALDERLAKE_N:
                SIPIndex = GEN12_LP_CSR;
                break;
            case IGFX_XE_HP_SDV:
                SIPIndex = XE_HP_CSR;
                [[fallthrough]];
            case IGFX_DG2:
            case IGFX_PVC:
            case IGFX_METEORLAKE:
            case IGFX_ARROWLAKE:
            case IGFX_LUNARLAKE:
            case IGFX_BMG :
                if (sysInfo.SLMSizeInKb == SLM_128)
                {
                    SIPIndex = XE2_CSR_DEBUG_BINDLESS_config128;
                    Xe2SIP_WMTP_CSRDebugBindlessDebugHeader.total_wmtp_data_size = XE2_CSR_DEBUG_BINDLESS_config128_WMTP_DATA_SIZE;
                }
                else if (sysInfo.SLMSizeInKb == SLM_160)
                {
                    SIPIndex = XE2_CSR_DEBUG_BINDLESS_config160;
                    Xe2SIP_WMTP_CSRDebugBindlessDebugHeader.total_wmtp_data_size = XE2_CSR_DEBUG_BINDLESS_config160_WMTP_DATA_SIZE;
                }
                else
                {
                    IGC_ASSERT(false);
                }
                break;

            default:
                break;
            }
        }
        else if (mode == SYSTEM_THREAD_MODE_CSR_64B)
        {
            break;
        }
        break;
    }

    case IGFX_XE3_CORE:
        if (mode & SYSTEM_THREAD_MODE_DEBUG)
        {
            SIPIndex = XE3G_DEBUG_BINDLESS;
        }
        else if(mode & SYSTEM_THREAD_MODE_CSR)
        {
            if(sysInfo.SliceCount == 1 && sysInfo.SubSliceCount == 4)
            {
                SIPIndex = XE3_CSR_DEBUG_BINDLESS_config_1x4;
            }
            //else if(sysInfo.SliceCount == 2 && sysInfo.SubSliceCount == 6)  or default case
            {
                SIPIndex = XE3_CSR_DEBUG_BINDLESS_config_2x6;
            }
        }
        break;

    default:
        IGC_ASSERT(0);
        break;
    }

    if (IGC_IS_FLAG_ENABLED(EnableSIPOverride))
    {
        std::string sipFile(IGC_GET_REGKEYSTRING(SIPOverrideFilePath));
        if (!sipFile.empty())
        {
            llvm::MemoryBuffer* pBuffer = LoadFile(sipFile);
            if (pBuffer)
            {
                m_LinearAddress = (void *)pBuffer->getBuffer().data();
                m_ProgramSize = pBuffer->getBufferSize();
            }
            else
            {
                IGC_ASSERT(false);
            }
        }
    }
    else
    {
        IGC_ASSERT_MESSAGE((SIPIndex < SIPKernelInfo.size()), "Invalid SIPIndex while loading");
        std::tie(m_LinearAddress, m_ProgramSize, m_StateSaveHeaderAddress, m_StateSaveHeaderSize) = SIPKernelInfo[SIPIndex];
    }

    if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable) && m_LinearAddress && (m_ProgramSize > 0))
    {
        std::string dumpFolder = IGC::Debug::GetShaderOutputFolder();

        IGC::Debug::DumpLock();
        std::string FileName = dumpFolder + "SIPKernelDump.bin";

        FILE* pFile = fopen(FileName.c_str(), "wb");

        if (pFile)
        {
            fwrite(m_LinearAddress, sizeof(char), m_ProgramSize, pFile);
            fclose(pFile);
        }

        IGC::Debug::DumpUnlock();
    }
    return NULL;
}

void CGenSystemInstructionKernelProgram::Delete(CGenSystemInstructionKernelProgram* &pKernelProgram )
{
    TODO("Release something if required later");
}

CGenSystemInstructionKernelProgram::CGenSystemInstructionKernelProgram(
    const SYSTEM_THREAD_MODE mode)
{
    m_LinearAddress = NULL;
    m_ProgramSize = 0 ;
    m_StateSaveHeaderAddress = NULL;
    m_StateSaveHeaderSize = 0;
}

}
