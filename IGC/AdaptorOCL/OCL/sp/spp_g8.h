/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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

namespace iOpenCL
{

// This is the base class to create an OpenCL ELF binary with patch tokens.
// It owns BinaryStreams allocated.
class CGen8OpenCLProgramBase : DisallowCopy {
public:
    explicit CGen8OpenCLProgramBase(PLATFORM platform,
                                    const CGen8OpenCLStateProcessor::IProgramContext& PI,
                                    const WA_TABLE& WATable);
    virtual ~CGen8OpenCLProgramBase();

    /// GetProgramBinary - getting legacy (Patch token based) binary format
    /// Write program header and the already written patch token info
    /// and kernels' binary to programBinary. Must be called after
    /// CGen8OpenCLProgram::CreateKernelBinaries or CGen8CMProgram::CreateKernelBinaries
    RETVAL GetProgramBinary(Util::BinaryStream& programBinary,
        unsigned pointerSizeInBytes);
    /// GetProgramDebugDataSize - get size of debug info patch token
    RETVAL GetProgramDebugDataSize(size_t& totalDbgInfoBufferSize);
    /// GetProgramDebugData - get debug data binary for legacy (Patch token based)
    /// binary format
    RETVAL GetProgramDebugData(char* dstBuffer, size_t dstBufferSize);
    /// GetProgramDebugData - get program debug data API used by VC.
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
    CGen8OpenCLProgram(PLATFORM platform, const IGC::OpenCLProgramContext &context);

    ~CGen8OpenCLProgram();

    /// API for getting legacy (Patch token based) binary
    /// CreateKernelBinaries - write patch token information and gen binary to
    /// m_ProgramScopePatchStream and m_KernelBinaries
    void CreateKernelBinaries();

    /// getZEBinary - create and get ZE Binary
    /// if spv and spvSize are given, a .spv section will be created in the output ZEBinary
    void GetZEBinary(
        llvm::raw_pwrite_stream& programBinary,
        unsigned pointerSizeInBytes,
        const char* spv,          uint32_t spvSize,
        const char* metrics,      uint32_t metricsSize,
        const char* buildOptions, uint32_t buildOptionsSize);

    // Used to track the kernel info from CodeGen
    std::vector<IGC::CShaderProgram*> m_ShaderProgramList;

private:

    class CLProgramCtxProvider : public CGen8OpenCLStateProcessor::IProgramContext {
    public:
        CLProgramCtxProvider(const IGC::OpenCLProgramContext& CtxIn): m_Context{CtxIn} {}

        ShaderHash getProgramHash() const override;
        bool needsSystemKernel() const  override;
        bool isProgramDebuggable() const override;
        bool hasProgrammableBorderColor() const override;
        bool useBindlessMode() const override;
        bool useBindlessLegacyMode() const override;


    private:
       const IGC::OpenCLProgramContext& m_Context;
    };

    const IGC::OpenCLProgramContext& m_Context;
    CLProgramCtxProvider m_ContextProvider;
};

}

#endif //SPP_G8_H
