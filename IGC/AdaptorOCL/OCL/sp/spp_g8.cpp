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

#include "spp_g8.h"

#include "../../../Compiler/CodeGenPublic.h"
#include "program_debug_data.h"

namespace iOpenCL
{

extern RETVAL g_cInitRetValue;

CGen8OpenCLProgram::CGen8OpenCLProgram(PLATFORM platform, const IGC::OpenCLProgramContext &context) :
    m_StateProcessor( platform, context ),
    m_Platform( platform )
{
    m_ProgramScopePatchStream = new Util::BinaryStream();
}

CGen8OpenCLProgram::~CGen8OpenCLProgram()
{
    while( m_KernelBinaries.empty() == false )
    {
        Util::BinaryStream* stream = m_KernelBinaries.back();

        delete stream;

        m_KernelBinaries.pop_back();
    }

    delete m_ProgramScopePatchStream;

    while( m_KernelDebugDataList.empty() == false )
    {
        Util::BinaryStream* stream = m_KernelDebugDataList.back();

        delete stream;

        m_KernelDebugDataList.pop_back();
    }
}

RETVAL CGen8OpenCLProgram::GetProgramBinary(
    Util::BinaryStream& programBinary,
    unsigned int pointerSizeInBytes )
{
    RETVAL retValue = g_cInitRetValue;

    iOpenCL::SProgramBinaryHeader   header;

    memset( &header, 0, sizeof( header ) );

    header.Magic = iOpenCL::MAGIC_CL;
    header.Version = iOpenCL::CURRENT_ICBE_VERSION;
    header.Device = m_Platform.eRenderCoreFamily;
    header.GPUPointerSizeInBytes = pointerSizeInBytes;
    header.NumberOfKernels = m_KernelBinaries.size();
    header.SteppingId = m_Platform.usRevId;
    header.PatchListSize = int_cast<DWORD>(m_ProgramScopePatchStream->Size());
    
    if (IGC_IS_FLAG_ENABLED(DumpOCLProgramInfo))
    {
        DebugProgramBinaryHeader(&header, m_StateProcessor.m_oclStateDebugMessagePrintOut);
    }

    programBinary.Write( header );

    programBinary.Write( *m_ProgramScopePatchStream );

    for( auto i = m_KernelBinaries.begin(); i != m_KernelBinaries.end(); ++i )
    {
        Util::BinaryStream* kernelBinary = *i;
        programBinary.Write( *kernelBinary );
    }

    return retValue;
}

RETVAL CGen8OpenCLProgram::GetProgramDebugData(Util::BinaryStream& programDebugData)
{
    RETVAL retValue = g_cInitRetValue;

    if( !m_KernelDebugDataList.empty() )
    {
        iOpenCL::SProgramDebugDataHeaderIGC header;

        memset( &header, 0, sizeof( header ) );

        header.Magic = iOpenCL::MAGIC_CL;
        header.Version = iOpenCL::CURRENT_ICBE_VERSION;
        header.Device = m_Platform.eRenderCoreFamily;
        header.NumberOfKernels = m_KernelDebugDataList.size();
        header.SteppingId = m_Platform.usRevId;


        programDebugData.Write( header );

        for( auto i = m_KernelDebugDataList.begin(); i != m_KernelDebugDataList.end(); ++i )
        {
            Util::BinaryStream* kernelDebugData = *i;
            programDebugData.Write( *kernelDebugData );
        }
    }

    return retValue;
}

void CGen8OpenCLProgram::AddKernelBinary(
        const char*  rawIsaBinary,
        unsigned int rawIsaBinarySize,
        const IGC::SOpenCLKernelInfo& kernelInfo,
        const IGC::SOpenCLProgramInfo& programInfo,
        const IGC::CBTILayout& layout,
        USC::SSystemThreadKernelOutput* pSystemThreadKernelOutput,
        unsigned int unpaddedBinarySize )
{
    Util::BinaryStream* kernelHeap = new Util::BinaryStream();

    m_StateProcessor.CreateKernelBinary(
        rawIsaBinary,
        rawIsaBinarySize,
        kernelInfo,
        programInfo,
        layout,
        *kernelHeap,
        pSystemThreadKernelOutput,
        unpaddedBinarySize);

    m_KernelBinaries.push_back( kernelHeap );
}

void CGen8OpenCLProgram::CreateProgramScopePatchStream(const IGC::SOpenCLProgramInfo& annotations)
{
    m_StateProcessor.CreateProgramScopePatchStream(annotations, *m_ProgramScopePatchStream);
}

void CGen8OpenCLProgram::AddKernelDebugData(
    const char*  rawDebugDataVISA,
    unsigned int rawDebugDataVISASize,
    const char*  rawDebugDataGenISA,
    unsigned int rawDebugDataGenISASize,
    const std::string& kernelName)
{
    if( rawDebugDataVISASize > 0 && rawDebugDataGenISASize > 0 )
    {
        Util::BinaryStream* kernelDebugData = new Util::BinaryStream();

        m_StateProcessor.CreateKernelDebugData(
            rawDebugDataVISA,
            rawDebugDataVISASize,
            rawDebugDataGenISA,
            rawDebugDataGenISASize,
            kernelName,
            *kernelDebugData);

        m_KernelDebugDataList.push_back( kernelDebugData );
    }
}

}
