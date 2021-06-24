/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SystemThread.h"
#include "common/allocator.h"
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
#include "common/SIPKernels/Gen12LPSIPCSRDebugBindlessDebugHeader.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace USC;

namespace SIP
{

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
            (void*)&Gen12LPSIPCSRDebugBindlessDebugHeader,
            (int)sizeof(Gen12LPSIPCSRDebugBindlessDebugHeader));

    GT_SYSTEM_INFO sysInfo = platform.GetGTSystemInfo();
    Gen12LPSIPCSRDebugBindlessDebugHeader.regHeader.num_slices = sysInfo.MaxSlicesSupported;
    Gen12LPSIPCSRDebugBindlessDebugHeader.regHeader.num_subslices_per_slice = sysInfo.MaxSubSlicesSupported;
    Gen12LPSIPCSRDebugBindlessDebugHeader.regHeader.num_eus_per_subslice = sysInfo.MaxEuPerSubSlice;
    Gen12LPSIPCSRDebugBindlessDebugHeader.regHeader.num_threads_per_eu = 0;

    // Avoid division by zero error in case if any of the sysInfo parameter is zero.
    if ((sysInfo.MaxEuPerSubSlice * sysInfo.MaxSubSlicesSupported * sysInfo.MaxSlicesSupported) != 0)
        Gen12LPSIPCSRDebugBindlessDebugHeader.regHeader.num_threads_per_eu =
            sysInfo.ThreadCount / (sysInfo.MaxEuPerSubSlice * sysInfo.MaxSubSlicesSupported * sysInfo.MaxSlicesSupported);

    if (sizeof(StateSaveAreaHeader) % 16)
	    Gen12LPSIPCSRDebugBindlessDebugHeader.regHeader.state_area_offset =
		16 - sizeof(StateSaveAreaHeader) % 16;

    // Match SIP alignment of debug surface
    IGC_ASSERT_MESSAGE(((Gen12LPSIPCSRDebugBindlessDebugHeader.regHeader.state_area_offset +
	  Gen12LPSIPCSRDebugBindlessDebugHeader.versionHeader.size) / 16 == 0x15),
	"Gen12 Bindless SIP header size alignment mismatch.");

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
    {
        if (mode & SYSTEM_THREAD_MODE_DEBUG)
        {
            SIPIndex = bindlessMode ? GEN12_LP_CSR_DEBUG_BINDLESS : GEN12_LP_CSR_DEBUG;
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
                SIPIndex = GEN12_LP_CSR;
                break;


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
