/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

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

using namespace llvm;
using namespace USC;

namespace SIP
{

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

    static constexpr size_t SR_COUNT = 2;
    static constexpr size_t SR_ELEMENTS = 4;
    static constexpr size_t SR_ELEMENT_SIZE = 4;
    static constexpr size_t SR_ALIGN = 0;

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
    {"tssarea", 0, {2, 0, 0}, sizeof(StateSaveAreaHeader) / 8, {0, 0, 0}},  // versionHeader
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
            pKernelProgram->Create( platform, mode, bindlessMode );

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
            (int)sizeof(Gen12SIPCSRDebugBindlessDebugHeader));

    SIPKernelInfo[XE_HP_CSR_DEBUG_BINDLESS] = std::make_tuple((void*)&XeHPSIPCSRDebugBindless,
        (int)sizeof(XeHPSIPCSRDebugBindless),
        (void*)&Gen12SIPCSRDebugBindlessDebugHeader,
        (int)sizeof(Gen12SIPCSRDebugBindlessDebugHeader));

    SIPKernelInfo[XE_HPG_CSR_DEBUG_BINDLESS] = std::make_tuple((void*)&XeHPGSIPCSRDebugBindless,
            (int)sizeof(XeHPGSIPCSRDebugBindless),
            (void*)&Gen12SIPCSRDebugBindlessDebugHeader,
            (int)sizeof(Gen12SIPCSRDebugBindlessDebugHeader));

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
}

CGenSystemInstructionKernelProgram* CGenSystemInstructionKernelProgram::Create(
    const IGC::CPlatform &platform,
    const SYSTEM_THREAD_MODE mode,
    const bool bindlessMode)
{
    llvm::MemoryBuffer* pBuffer = nullptr;
    unsigned char SIPIndex = 0;
    std::map< unsigned char, std::tuple<void*, unsigned int, void*, unsigned int> > SIPKernelInfo;
    populateSIPKernelInfo(platform, SIPKernelInfo);

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
    {
        if (mode & SYSTEM_THREAD_MODE_DEBUG)
        {
            switch (platform.getPlatformInfo().eProductFamily)
            {
            case IGFX_TIGERLAKE_LP:
            case IGFX_DG1:
            case IGFX_ROCKETLAKE:
            case IGFX_ALDERLAKE_S:
                SIPIndex = bindlessMode ? GEN12_LP_CSR_DEBUG_BINDLESS : GEN12_LP_CSR_DEBUG;
                break;
            case IGFX_XE_HP_SDV:
                SIPIndex = bindlessMode ? XE_HP_CSR_DEBUG_BINDLESS : XE_HP_CSR_DEBUG;
                break;
            case IGFX_DG2:
            case IGFX_PVC:

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
                SIPIndex = GEN12_LP_CSR;
                break;
            case IGFX_XE_HP_SDV:
                SIPIndex = XE_HP_CSR;
            case IGFX_DG2:
            case IGFX_PVC:

            default:
                break;
            }
        }
        break;
    }
    default:
        IGC_ASSERT(0);
        break;
    }

    if (IGC_IS_FLAG_ENABLED(EnableSIPOverride))
    {
        std::string sipFile(IGC_GET_REGKEYSTRING(SIPOverrideFilePath));
        if (!sipFile.empty())
        {
            pBuffer = LoadFile(sipFile);
        }
        IGC_ASSERT(pBuffer);
        if (pBuffer)
        {
            m_LinearAddress = (void *)pBuffer->getBuffer().data();
            m_ProgramSize = pBuffer->getBufferSize();
        }
    }
    else
    {
        IGC_ASSERT_MESSAGE((SIPIndex < SIPKernelInfo.size()), "Invalid SIPIndex while loading");
        m_LinearAddress = std::get<0>(SIPKernelInfo[SIPIndex]);
        m_ProgramSize   = std::get<1>(SIPKernelInfo[SIPIndex]);
        m_StateSaveHeaderAddress = std::get<2>(SIPKernelInfo[SIPIndex]);
        m_StateSaveHeaderSize = std::get<3>(SIPKernelInfo[SIPIndex]);
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
