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

        if( mode & SYSTEM_THREAD_MODE_CSR )
        {
            unsigned int subSliceCount = Caps.KernelHwCaps.SubSliceCount;
            // HSW and BDW differs in EU count per subslice but during preemption uses same memory size for EU data dump
            const unsigned int EuRangeInSubslice = 0x10;
            // HSW and BDW uses the same range for dumping data from single EU, 64KB although there are 7 threads in EU
            const unsigned int singleEuDumpSize = 0x10000UL; // 64KB
            const unsigned int slmSingleSubsliceData = 0x10000UL; // 64KB

            // during preemption, data which should be preserved, are stored in specialized preemption scratch space,
            // scratch space contains dump for SLM memory for each subslice and all spawned EU threads (GRFs and ARFs)
            //
            // EU data: 8k per each thread, ordered by FFTid, preemption EU design specify 64KB space per EU 
            //          (although there are 7 threads in EU), additionaly EU data are alligned by rows,
            // SLM memory: 64k of SLM per each subslice
            pSystemThreadKernelOutput->m_SystemThreadScratchSpace = 
                subSliceCount *
                EuRangeInSubslice *                                                              
                singleEuDumpSize                                  // EU data
                + 
                ( subSliceCount * slmSingleSubsliceData );        // SLM data                                  
        }

        if( mode & ( SYSTEM_THREAD_MODE_DEBUG | SYSTEM_THREAD_MODE_DEBUG_LOCAL ) )
        {
            unsigned int subSliceCount = Caps.KernelHwCaps.SubSliceCount;
            const unsigned int slmSingleSubsliceData = 0x10000UL; // 64KB

            pSystemThreadKernelOutput->m_SystemThreadResourceSize =
                pKernelProgram->GetCacheFlushDataSize() + 
                pKernelProgram->GetPerThreadDebugDataSize() * 
                Caps.KernelHwCaps.EUCount * 
                Caps.KernelHwCaps.EUThreadsPerEU
                +
                (subSliceCount * slmSingleSubsliceData);        // SLM data                                  

        }

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
    SIPKernelInfo[GEN9_SIP_DEBUG] = std::make_pair((void*)&Gen9SIPDebug, sizeof(Gen9SIPDebug));

    SIPKernelInfo[GEN9_SIP_CSR] = std::make_pair((void*)&Gen9SIPCSR, sizeof(Gen9SIPCSR));

    SIPKernelInfo[GEN9_SIP_CSR_DEBUG] = std::make_pair((void*)&Gen9SIPCSRDebug, sizeof(Gen9SIPCSRDebug));

    SIPKernelInfo[GEN10_SIP_DEBUG] = std::make_pair((void*)&Gen10SIPDebug, sizeof(Gen10SIPDebug));

    SIPKernelInfo[GEN10_SIP_CSR] = std::make_pair((void*)&Gen10SIPCSR, sizeof(Gen10SIPCSR));

    SIPKernelInfo[GEN10_SIP_CSR_DEBUG] = std::make_pair((void*)&Gen10SIPCSRDebug, sizeof(Gen10SIPCSRDebug));

    SIPKernelInfo[GEN9_SIP_DEBUG_BINDLESS] = std::make_pair((void*)&Gen9SIPDebugBindless, sizeof(Gen9SIPDebugBindless));

    SIPKernelInfo[GEN10_SIP_DEBUG_BINDLESS] = std::make_pair((void*)&Gen10SIPDebugBindless, sizeof(Gen10SIPDebugBindless));

    SIPKernelInfo[GEN9_BXT_SIP_CSR] = std::make_pair((void*)&Gen9BXTSIPCSR, sizeof(Gen9BXTSIPCSR));

    SIPKernelInfo[GEN9_SIP_CSR_DEBUG_LOCAL] = std::make_pair((void*)&Gen9SIPCSRDebugLocal, sizeof(Gen9SIPCSRDebugLocal));
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
    TODO("Switch if required based on platform");
    m_pGpgpuArfSaveRegData = &cGpgpuArfSaveRegDataGen8[ 0 ] ;
    m_pGpgpuArfRestoreRegData = &cGpgpuArfRestoreRegDataGen8[ 0 ] ;
    m_cGpgpuArfRegNumber = sizeof( cGpgpuArfSaveRegDataGen8 ) / sizeof( SArfRegData );

    if( mode & ( SYSTEM_THREAD_MODE_DEBUG | SYSTEM_THREAD_MODE_DEBUG_LOCAL ) )
    {
        m_pShaderDebugArfRegData = &cShaderDebugArfRegDataGen8[ 0 ];
        m_cShaderDebugArfRegNumber = sizeof( cShaderDebugArfRegDataGen8 ) / sizeof( SArfRegData );
        m_PerThreadDebugDataSizeInBytes = cGen8SIPThreadScratchSize;
    }

    m_FlushDataSizeInBytes = 0;
    m_CacheFlushCount = 0 ;
    m_LinearAddress = NULL;
    m_ProgramSize = 0 ;
}

}
