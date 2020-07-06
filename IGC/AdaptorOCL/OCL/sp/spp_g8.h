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

#ifndef SPP_G8_H
#define SPP_G8_H

#pragma once

#include <memory>
#include "../util/BinaryStream.h"
#include "usc.h"
#include "sp_g8.h"
#include "zebin_builder.hpp"

namespace IGC
{
    class OpenCLProgramContext;
    class CShaderProgram;
    class COCLBTILayout;
    struct SOpenCLProgramInfo;
    struct SProgramOutput;
};

namespace cmc
{
    class CMKernel;
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

    /// GetProgramBinary - getting legacy (Patch token based) binary format
    /// Write program header and the already written patch token info
    /// and kernels' binary to programBinary. Must be called after
    /// CGen8OpenCLProgram::CreateKernelBinaries or CGen8CMProgram::CreateKernelBinaries
    RETVAL GetProgramBinary(Util::BinaryStream& programBinary,
        unsigned pointerSizeInBytes);
    /// GetProgramDebugData - get debug data binary for legacy (Patch token based)
    /// binary format
    RETVAL GetProgramDebugData(Util::BinaryStream& programDebugData);
    /// CreateProgramScopePatchStream - get program scope patch token for legacy
    /// (Patch token based) binary format
    void CreateProgramScopePatchStream(const IGC::SOpenCLProgramInfo& programInfo);

    // For per-kernel binary streams and kernelInfo
    std::vector<KernelData> m_KernelBinaries;

    USC::SSystemThreadKernelOutput* m_pSystemThreadKernelOutput = nullptr;

    PLATFORM getPlatform() const { return m_Platform; }

public:
    // GetZEBinary - get ZE binary object
    virtual void GetZEBinary(llvm::raw_pwrite_stream& programBinary,
        unsigned pointerSizeInBytes) {}

protected:
    PLATFORM m_Platform;
    CGen8OpenCLStateProcessor m_StateProcessor;
    // For serialized patch token information
    Util::BinaryStream* m_ProgramScopePatchStream = nullptr;
};

class CGen8OpenCLProgram : public CGen8OpenCLProgramBase
{
public:
    CGen8OpenCLProgram(PLATFORM platform, IGC::OpenCLProgramContext &context);

    ~CGen8OpenCLProgram();

    /// API for getting legacy (Patch token based) binary
    /// CreateKernelBinaries - write patch token information and gen binary to
    /// m_ProgramScopePatchStream and m_KernelBinaries
    void CreateKernelBinaries();

    /// getZEBinary - create and get ZE Binary
    void GetZEBinary(llvm::raw_pwrite_stream& programBinary, unsigned pointerSizeInBytes);

    // Used to track the kernel info from CodeGen
    std::vector<IGC::CShaderProgram*> m_ShaderProgramList;

private:
    IGC::OpenCLProgramContext* m_pContext = nullptr;
};

class CGen8CMProgram : public CGen8OpenCLProgramBase {
public:
    explicit CGen8CMProgram(PLATFORM platform);
    ~CGen8CMProgram();

    // Produce the final ELF binary with the given CM kernels
    // in OpenCL format.
    void CreateKernelBinaries();
    void GetZEBinary(llvm::raw_pwrite_stream& programBinary,
        unsigned pointerSizeInBytes) override;

    // CM kernel list.
    std::vector<cmc::CMKernel*> m_kernels;

    // Data structure to create patch token based binaries.
    std::unique_ptr<IGC::SOpenCLProgramInfo> m_programInfo;
};
}

#endif //SPP_G8_H
