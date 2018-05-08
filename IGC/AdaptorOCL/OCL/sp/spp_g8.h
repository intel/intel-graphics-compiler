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
#include <llvm/ADT/MapVector.h>
#include <llvm/IR/Function.h>
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
    const IGC::SOpenCLKernelInfo* pKernelInfo = nullptr;
    Util::BinaryStream* kernelBinary = nullptr;
    Util::BinaryStream* kernelDebugData = nullptr;
};

class CGen8OpenCLProgram : DisallowCopy
{
public:
    CGen8OpenCLProgram(PLATFORM platform, IGC::OpenCLProgramContext &context);

    ~CGen8OpenCLProgram();

    void ClearKernelOutput();

    RETVAL GetProgramBinary(
        Util::BinaryStream& programBinary,
        unsigned int pointerSizeInBytes );

    RETVAL GetProgramDebugData(Util::BinaryStream& programDebugData);

    void CreateKernelBinaries();

    void CreateProgramScopePatchStream(const IGC::SOpenCLProgramInfo& programInfo);

    // Used to track the kernel info from CodeGen
    llvm::MapVector<llvm::Function*, IGC::CShaderProgram*> m_KernelShaderMap;
    USC::SSystemThreadKernelOutput* m_pSystemThreadKernelOutput = nullptr;

    // Used to store per-kernel binary streams and kernelInfo
    std::vector<KernelData> m_KernelBinaries;

private:
    CGen8OpenCLStateProcessor m_StateProcessor;
    Util::BinaryStream* m_ProgramScopePatchStream;
    PLATFORM  m_Platform;
    IGC::OpenCLProgramContext* m_pContext;
};


}
