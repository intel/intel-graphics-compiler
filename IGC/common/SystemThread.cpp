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

#include "SystemThread.h"
#include "common/allocator.h"
#include "common/igc_regkeys.hpp"
#include "common/secure_mem.h"
#include "common/debug/Dump.hpp"

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
        ASSERT( success );
    }

    // Create System Thread kernel program.
    if( success )
    {
        CGenSystemInstructionKernelProgram* pKernelProgram =
            new CGenSystemInstructionKernelProgram(mode);
        success = pKernelProgram ? true : false;
        ASSERT( success );

        // Allocate memory for SSystemThreadKernelOutput.
        if( success )
        {
            ASSERT( pSystemThreadKernelOutput == nullptr );
            pSystemThreadKernelOutput = new SSystemThreadKernelOutput;

            success = ( pSystemThreadKernelOutput != nullptr );
            ASSERT( success );

            if( success )
            {
                memset(
                    pSystemThreadKernelOutput,
                    0 ,
                    sizeof( SSystemThreadKernelOutput ) );
            }
        }

        const unsigned int DQWORD_SIZE = 2 * sizeof( unsigned long long );

        pKernelProgram->Create(platform, mode, bindlessMode);

        pSystemThreadKernelOutput->m_KernelProgramSize =  pKernelProgram->GetProgramSize();

        const IGC::SCompilerHwCaps &Caps = const_cast<IGC::CPlatform&>(platform).GetCaps();

        if (mode & SYSTEM_THREAD_MODE_CSR)
        pSystemThreadKernelOutput->m_SystemThreadScratchSpace = Caps.KernelHwCaps.CsrSizeInMb * sizeof(MEGABYTE);

        if (mode & (SYSTEM_THREAD_MODE_DEBUG | SYSTEM_THREAD_MODE_DEBUG_LOCAL))
        pSystemThreadKernelOutput->m_SystemThreadResourceSize = Caps.KernelHwCaps.CsrSizeInMb * 2 * sizeof(MEGABYTE);

        pSystemThreadKernelOutput->m_pKernelProgram =
            IGC::aligned_malloc( pSystemThreadKernelOutput->m_KernelProgramSize, DQWORD_SIZE );

        success = ( pSystemThreadKernelOutput->m_pKernelProgram != nullptr );

        if( success )
        {
            void* pStartAddress = pKernelProgram->GetLinearAddress();

            if( !pStartAddress )
            {
                ASSERT( 0 );
                success = false;
            }

            if( success )
            {
                memcpy_s(
                    pSystemThreadKernelOutput->m_pKernelProgram,
                    pSystemThreadKernelOutput->m_KernelProgramSize,
                    pStartAddress,
                    pSystemThreadKernelOutput->m_KernelProgramSize );
            }
        }
        else
        {
            ASSERT( false );
            success = false;
        }
        pKernelProgram->Delete( pKernelProgram );
        delete pKernelProgram;
    }
    return success;
}


void CSystemThread::DeleteSystemThreadKernel(
    USC::SSystemThreadKernelOutput* &pSystemThreadKernelOutput )
{
    if (pSystemThreadKernelOutput->m_pKernelProgram &&
        (pSystemThreadKernelOutput->m_KernelProgramSize > 0) )
    {
        IGC::aligned_free(pSystemThreadKernelOutput->m_pKernelProgram);
        delete pSystemThreadKernelOutput;
    }
}

//populate the SIPKernelInfo map with starting address and size of every SIP kernels
void populateSIPKernelInfo(std::map< unsigned char, std::pair<void*, unsigned int> > &SIPKernelInfo)
{
    //LLVM_UPGRADE_TODO
    // check if (int)sizeof(T) is ok or change the pair def for SIPKernelInfo
    SIPKernelInfo[GEN9_SIP_DEBUG] = std::make_pair((void*)&Gen9SIPDebug, (int)sizeof(Gen9SIPDebug));

    SIPKernelInfo[GEN9_SIP_CSR] = std::make_pair((void*)&Gen9SIPCSR, (int)sizeof(Gen9SIPCSR));

    SIPKernelInfo[GEN9_SIP_CSR_DEBUG] = std::make_pair((void*)&Gen9SIPCSRDebug, (int)sizeof(Gen9SIPCSRDebug));

    SIPKernelInfo[GEN10_SIP_DEBUG] = std::make_pair((void*)&Gen10SIPDebug, (int)sizeof(Gen10SIPDebug));

    SIPKernelInfo[GEN10_SIP_CSR] = std::make_pair((void*)&Gen10SIPCSR, (int)sizeof(Gen10SIPCSR));

    SIPKernelInfo[GEN10_SIP_CSR_DEBUG] = std::make_pair((void*)&Gen10SIPCSRDebug, (int)sizeof(Gen10SIPCSRDebug));

    SIPKernelInfo[GEN9_SIP_DEBUG_BINDLESS] = std::make_pair((void*)&Gen9SIPDebugBindless, (int)sizeof(Gen9SIPDebugBindless));

    SIPKernelInfo[GEN10_SIP_DEBUG_BINDLESS] = std::make_pair((void*)&Gen10SIPDebugBindless, (int)sizeof(Gen10SIPDebugBindless));

    SIPKernelInfo[GEN9_BXT_SIP_CSR] = std::make_pair((void*)&Gen9BXTSIPCSR, (int)sizeof(Gen9BXTSIPCSR));

    SIPKernelInfo[GEN9_SIP_CSR_DEBUG_LOCAL] = std::make_pair((void*)&Gen9SIPCSRDebugLocal, (int)sizeof(Gen9SIPCSRDebugLocal));

    SIPKernelInfo[GEN9_GLV_SIP_CSR] = std::make_pair((void*)&Gen9GLVSIPCSR, (int)sizeof(Gen9GLVSIPCSR));

    SIPKernelInfo[GEN11_SIP_CSR] = std::make_pair((void*)&Gen11SIPCSR, (int)sizeof(Gen11SIPCSR));

    SIPKernelInfo[GEN11_HP_SIP_CSR] = std::make_pair((void*)&Gen11HPSIPCSR, (int)sizeof(Gen11HPSIPCSR));

    SIPKernelInfo[GEN11_LKF_SIP_CSR] = std::make_pair((void*)&Gen11LKFSIPCSR, (int)sizeof(Gen11LKFSIPCSR));

    SIPKernelInfo[GEN12_LP_CSR] = std::make_pair((void*)&Gen12LPSIPCSR, (int)sizeof(Gen12LPSIPCSR));
}

CGenSystemInstructionKernelProgram* CGenSystemInstructionKernelProgram::Create(
    const IGC::CPlatform &platform,
    const SYSTEM_THREAD_MODE mode,
    const bool bindlessMode)
{
    llvm::MemoryBuffer* pBuffer = nullptr;
    unsigned char SIPIndex = 0;
    std::map< unsigned char, std::pair<void*, unsigned int> > SIPKernelInfo;
    populateSIPKernelInfo(SIPKernelInfo);

    switch( platform.getPlatformInfo().eRenderCoreFamily )
    {
    case IGFX_GEN9_CORE:
    case IGFX_GENNEXT_CORE:
    {
        if(bindlessMode)
        {
            if(mode == SYSTEM_THREAD_MODE_DEBUG)
            {
                SIPIndex = GEN9_SIP_DEBUG_BINDLESS;
            }
            //Add the rest later for bindless mode for preemption
        }
        else if(mode == SYSTEM_THREAD_MODE_DEBUG)
        {
            SIPIndex =  GEN9_SIP_DEBUG;
        }
        else if(mode == SYSTEM_THREAD_MODE_CSR)
        {
            if ((platform.getPlatformInfo().eProductFamily == IGFX_BROXTON) ||
                (platform.getPlatformInfo().eProductFamily == IGFX_GEMINILAKE))
            {
                /*Special SIP for 2x6 from HW team with 64KB offset*/
                SIPIndex = GEN9_BXT_SIP_CSR;
            }
            else
            {
                SIPIndex = GEN9_SIP_CSR;
            }
        }
        else if(mode == (SYSTEM_THREAD_MODE_CSR | SYSTEM_THREAD_MODE_DEBUG))
        {
            SIPIndex = GEN9_SIP_CSR_DEBUG;
        }
        else if (mode == (SYSTEM_THREAD_MODE_CSR | SYSTEM_THREAD_MODE_DEBUG_LOCAL))
        {
            SIPIndex = GEN9_SIP_CSR_DEBUG_LOCAL;
        }
        break;
    }
    case IGFX_GEN10_CORE:
    {
        if (bindlessMode)
        {
            if (mode == SYSTEM_THREAD_MODE_DEBUG)
            {
                SIPIndex = GEN10_SIP_DEBUG_BINDLESS;
            }
            //Add the rest later for bindless mode for preemption
        }
        else if (mode == SYSTEM_THREAD_MODE_DEBUG)
        {
            SIPIndex = GEN10_SIP_DEBUG;
        }
        else if (mode == SYSTEM_THREAD_MODE_CSR)
        {
            SIPIndex = GEN10_SIP_CSR;
        }
        else if (mode == (SYSTEM_THREAD_MODE_CSR | SYSTEM_THREAD_MODE_DEBUG))
        {
            SIPIndex = GEN10_SIP_CSR_DEBUG;
        }
        break;
    }
    case IGFX_GEN11_CORE:
    {
        if (bindlessMode)
        {
            if (mode == SYSTEM_THREAD_MODE_DEBUG)
            {
                SIPIndex = GEN10_SIP_DEBUG_BINDLESS;
            }
            //Add the rest later for bindless mode for preemption
        }
        else if (mode == SYSTEM_THREAD_MODE_DEBUG)
        {
            SIPIndex = GEN10_SIP_DEBUG;
        }
        else if (mode == SYSTEM_THREAD_MODE_CSR)
        {
            if (platform.getPlatformInfo().eProductFamily == IGFX_ICELAKE_LP)
            {
                SIPIndex = GEN11_SIP_CSR;
            }
            if (platform.getPlatformInfo().eProductFamily == IGFX_ICELAKE)
            {
                SIPIndex = GEN11_HP_SIP_CSR;
            }
            if ((platform.getPlatformInfo().eProductFamily == IGFX_LAKEFIELD)
             || (platform.getPlatformInfo().eProductFamily == IGFX_ELKHARTLAKE)
             || (platform.getPlatformInfo().eProductFamily == IGFX_JASPERLAKE)
            )
            {
                SIPIndex = GEN11_LKF_SIP_CSR;
            }
        }
        else if (mode == (SYSTEM_THREAD_MODE_CSR | SYSTEM_THREAD_MODE_DEBUG))
        {
            SIPIndex = GEN10_SIP_CSR_DEBUG;
        }
        break;
    }
    case IGFX_GEN12_CORE:
    case IGFX_GEN12LP_CORE:
    {
        if (bindlessMode)
        {
            if (mode == SYSTEM_THREAD_MODE_DEBUG)
            {
                SIPIndex =  GEN10_SIP_DEBUG_BINDLESS;
            }
            //Add the rest later for bindless mode for preemption
        }
        else if (mode == SYSTEM_THREAD_MODE_DEBUG)
        {
            SIPIndex = GEN10_SIP_DEBUG;
        }
        else if (mode == SYSTEM_THREAD_MODE_CSR)
        {
            if (platform.getPlatformInfo().eProductFamily == IGFX_TIGERLAKE_LP
            )
            {
                SIPIndex = GEN12_LP_CSR;
            }
        }
        else if (mode == (SYSTEM_THREAD_MODE_CSR | SYSTEM_THREAD_MODE_DEBUG))
        {
            SIPIndex = GEN10_SIP_CSR_DEBUG;
        }
        break;
    }
    default:
        ASSERT(0);
        break;
    }

    if (IGC_IS_FLAG_ENABLED(EnableSIPOverride))
    {
        std::string sipFile(IGC_GET_REGKEYSTRING(SIPOverrideFilePath));
        if (!sipFile.empty())
        {
            pBuffer = LoadFile(sipFile);
        }
        assert(pBuffer);
        if (pBuffer)
        {
            m_LinearAddress = (void *)pBuffer->getBuffer().data();
            m_ProgramSize = pBuffer->getBufferSize();
        }
    }
    else
    {
        assert(SIPIndex < SIPKernelInfo.size() && "Invalid SIPIndex while loading");
        m_LinearAddress = SIPKernelInfo[SIPIndex].first;
        m_ProgramSize = SIPKernelInfo[SIPIndex].second;
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
}

}
