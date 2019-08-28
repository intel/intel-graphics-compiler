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
    class CShaderProgram;
};

namespace iOpenCL
{

struct KernelData
{
    Util::BinaryStream* kernelBinary = nullptr;
    Util::BinaryStream* kernelDebugData = nullptr;
};

// This is the base class to create an OpenCL ELF binary with patch tokens.
// It owns BinaryStreams allocated.
class CGen8OpenCLProgramBase : DisallowCopy {
public:
    explicit CGen8OpenCLProgramBase(PLATFORM platform);
    virtual ~CGen8OpenCLProgramBase();

    RETVAL GetProgramBinary(Util::BinaryStream& programBinary,
        unsigned pointerSizeInBytes);

    RETVAL GetProgramDebugData(Util::BinaryStream& programDebugData);

    void CreateProgramScopePatchStream(const IGC::SOpenCLProgramInfo& programInfo);

    // Used to store per-kernel binary streams and kernelInfo
    std::vector<KernelData> m_KernelBinaries;

    USC::SSystemThreadKernelOutput* m_pSystemThreadKernelOutput = nullptr;

protected:
    PLATFORM m_Platform;
    CGen8OpenCLStateProcessor m_StateProcessor;
    Util::BinaryStream* m_ProgramScopePatchStream = nullptr;
};

class CGen8OpenCLProgram : public CGen8OpenCLProgramBase
{
public:
    CGen8OpenCLProgram(PLATFORM platform, IGC::OpenCLProgramContext &context);

    ~CGen8OpenCLProgram();

    void CreateKernelBinaries();

    // Used to track the kernel info from CodeGen
    std::vector<IGC::CShaderProgram*> m_ShaderProgramList;

private:
    IGC::OpenCLProgramContext* m_pContext = nullptr;
};

}
