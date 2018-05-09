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

#pragma once

#include "../util/BinaryStream.h"
#include "usc.h"
#include "sp_g8.h"

namespace IGC
{
    class OpenCLProgramContext;
};

namespace iOpenCL
{

class CGen8OpenCLProgram : DisallowCopy
{
public:
    CGen8OpenCLProgram(PLATFORM platform, const IGC::OpenCLProgramContext &context);

    ~CGen8OpenCLProgram();

    RETVAL GetProgramBinary(
        Util::BinaryStream& programBinary,
        unsigned int pointerSizeInBytes );

    RETVAL GetProgramDebugData(Util::BinaryStream& programDebugData);

    void AddKernelBinary(
        const char*  rawIsaBinary,
        unsigned int rawIsaBinarySize,
        const IGC::SOpenCLKernelInfo& kernelInfo,
        const IGC::SOpenCLProgramInfo& programInfo,
        const IGC::CBTILayout& layout,
        USC::SSystemThreadKernelOutput* pSystemThreadKernelOutput,
        unsigned int unpaddedBinarySize);

    void CreateProgramScopePatchStream(const IGC::SOpenCLProgramInfo& programInfo);

    void AddKernelDebugData(
        const char*  rawDebugDataVISA,
        unsigned int rawDebugDataVISASize,
        const char*  rawDebugDataGenISA,
        unsigned int rawDebugDataGenISASize,
        const std::string& kernelName);

private:
    CGen8OpenCLStateProcessor m_StateProcessor;
    std::vector<Util::BinaryStream*> m_KernelBinaries;
    Util::BinaryStream* m_ProgramScopePatchStream;
    std::vector<Util::BinaryStream*> m_KernelDebugDataList;
    PLATFORM  m_Platform;
};


}
