/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <vector>
#include <memory>

#include "../Platform/cmd_media_caps_g8.h"
#include "../../inc/common/igfxfmid.h"

#include "../CommandStream/SamplerTypes.h"
#include "../CommandStream/SurfaceTypes.h"

#include "../KernelAnnotations.hpp"
#include "../util/BinaryStream.h"

#include "../Patch/patch_parser.h"
#include "inc/common/Compiler/API/SurfaceFormats.h"
#include "common/shaderHash.hpp"
#include "usc.h"
#include "sp_debug.h"
#include "Probe/Assertion.h"

namespace IGC
{
    struct SOpenCLKernelInfo;
    struct SOpenCLProgramInfo;
    class CBTILayout;
    class OpenCLProgramContext;
}
namespace G6HWC
{
    struct SGfxSamplerIndirectState;
}

namespace iOpenCL
{

// Forward Declare
struct SStateProcessorContextGen8_0;


class KernelData
{
public:
    class DbgInfoBuffer
    {
    public:
        std::unique_ptr<Util::BinaryStream> header;
        const uint8_t* dbgInfoBuffer = nullptr;
        uint32_t dbgInfoBufferSize = 0;
        uint8_t extraAlignBytes = 0;
    };

    std::unique_ptr<Util::BinaryStream> kernelBinary;
    // DbgInfo is stored in pieces as header, dbg buffer, alignment padding
    // instead of storing it in a single BinaryStream. This is because
    // BinaryStream approach requires unnecessary copies.
    DbgInfoBuffer dbgInfo;

    // TODO: VC should switch to DbgInfoBuffer
    // kernelDebugData instance below is used only for VC
    std::unique_ptr<Util::BinaryStream> vcKernelDebugData;

    KernelData(KernelData &&Other) = default;
    KernelData() = default;
};

struct RETVAL
{
    DWORD Success;
};

extern RETVAL g_cInitRetValue;

class DisallowCopy
{
public:
    DisallowCopy() { };
    virtual ~DisallowCopy() {};

private:
    DisallowCopy( const DisallowCopy& );
    DisallowCopy& operator=(const DisallowCopy&);
};

/// CGen8OpenCLStateProcessor - Provides services to create (legacy) patch token
/// based binary from given SProgramOutput information
class CGen8OpenCLStateProcessor : DisallowCopy
{

public:
    class IProgramContext {
    public:
      virtual ShaderHash getProgramHash() const = 0;
      virtual bool needsSystemKernel() const  = 0;
      virtual bool isProgramDebuggable() const = 0;
      virtual bool hasProgrammableBorderColor() const = 0;
      virtual bool useBindlessMode() const = 0;
      virtual bool useBindlessLegacyMode() const = 0;

      virtual ~IProgramContext () {}
    };

    explicit CGen8OpenCLStateProcessor(PLATFORM platform, const IProgramContext& Ctx, const WA_TABLE& WATable);
    virtual ~CGen8OpenCLStateProcessor( void );

    virtual void CreateKernelBinary(
        const char*  rawIsaBinary,
        unsigned int rawIsaBinarySize,
        const IGC::SOpenCLKernelInfo& annotations,
        const IGC::SOpenCLProgramInfo& programInfo,
        const IGC::CBTILayout& layout,
        Util::BinaryStream& kernelHeap,
        USC::SSystemThreadKernelOutput* pSystemThreadKernelOutput,
        unsigned int unpaddedBinarySize);

    void CreateProgramScopePatchStream(const IGC::SOpenCLProgramInfo& annotations,
        Util::BinaryStream& membuf);

    virtual void CreateKernelDebugData(
        const char* rawDebugDataVISA,
        unsigned int rawDebugDataVISASize,
        const char* rawDebugDataGenISA,
        unsigned int rawDebugDataGenISASize,
        const std::string& kernelName,
        Util::BinaryStream& kernelDebugData);

    virtual void CreateKernelDebugData(
        const char*  rawDebugDataVISA,
        unsigned int rawDebugDataVISASize,
        const char*  rawDebugDataGenISA,
        unsigned int rawDebugDataGenISASize,
        const std::string& kernelName,
        KernelData::DbgInfoBuffer& kernelDebugData);

    const IProgramContext& m_Context;
    std::string m_oclStateDebugMessagePrintOut;
    const WA_TABLE& m_WATable;

private:
    const G6HWC::SMediaHardwareCapabilities& HWCaps() const;

    RETVAL AddSystemKernel(
        const USC::SSystemThreadKernelOutput* pSystemThreadKernelOutput,
        SStateProcessorContextGen8_0& context,
        Util::BinaryStream& membuf);

    RETVAL CreateKernelHeap(
        const IGC::SOpenCLKernelInfo& annotations,
        const char* kernelBinary,
        unsigned int kernelBinarySize,
        USC::SSystemThreadKernelOutput* pSystemThreadKernelOutput,
        SStateProcessorContextGen8_0& context,
        Util::BinaryStream& membuf );

    RETVAL CreateSurfaceStateHeap(
        const IGC::SOpenCLKernelInfo& annotations,
        const IGC::CBTILayout& layout,
        SStateProcessorContextGen8_0& context,
        Util::BinaryStream& );

    RETVAL CreateDynamicStateHeap(
        const IGC::SOpenCLKernelInfo& annotations,
        SStateProcessorContextGen8_0& context,
        Util::BinaryStream& );

    RETVAL CreateGeneralStateHeap(
        const IGC::SOpenCLKernelInfo& annotations,
        SStateProcessorContextGen8_0& context,
        Util::BinaryStream& );

    RETVAL CreatePatchList(
        const IGC::SOpenCLKernelInfo& annotations,
        const IGC::SOpenCLProgramInfo& programInfo,
        const IGC::CBTILayout& layout,
        const SStateProcessorContextGen8_0& context,
        Util::BinaryStream& );

    RETVAL CombineKernelBinary(
        const SStateProcessorContextGen8_0& context,
        const IGC::SOpenCLKernelInfo& annotations,
        const Util::BinaryStream& kernelHeap,
        const Util::BinaryStream& generalStateHeap,
        const Util::BinaryStream& dynamicStateHeap,
        const Util::BinaryStream& surfaceStateHeap,
        const Util::BinaryStream& patchListHeap,
        unsigned int unpaddedBinarySize,
        Util::BinaryStream& kernelBinary );

    template<class PatchType>
    RETVAL AddPatchItem(
        PatchType& patch,
        Util::BinaryStream& membuf )
    {
        RETVAL retValue = g_cInitRetValue;

        if( membuf.Write( patch ) == false )
        {
            retValue.Success = false;
        }

#if defined(_DEBUG) || defined(_INTERNAL) || defined(_RELEASE_INTERNAL)  || defined(ICBE_LINUX) || defined(_LINUX) || defined(LINUX)
        DebugPatchList(&patch, patch.Size, m_oclStateDebugMessagePrintOut);
#endif

        return retValue;
    }

    RETVAL AddSamplerState(
        const bool enable,
        const SAMPLER_TEXTURE_ADDRESS_MODE& addressModeX,
        const SAMPLER_TEXTURE_ADDRESS_MODE& addressModeY,
        const SAMPLER_TEXTURE_ADDRESS_MODE& addressModeZ,
        const SAMPLER_MAPFILTER_TYPE& filterMag,
        const SAMPLER_MAPFILTER_TYPE& filterMin,
        const SAMPLER_MIPFILTER_TYPE& filterMip,
        const bool normalizedCoords,
        const SAMPLER_COMPARE_FUNC_TYPE& compareFunc,
        const DWORD borderColorOffset,
        Util::BinaryStream& membuf );

    DWORD AllocateSamplerIndirectState(
        const G6HWC::SGfxSamplerIndirectState& borderColor,
        Util::BinaryStream &membuf);

    RETVAL AddSurfaceState(
        const SURFACE_TYPE& type,
        const IGC::SURFACE_FORMAT& surfaceFormat,
        const DWORD bufferLength,
        bool isMultiSampleImage,
        Util::BinaryStream& membuf );

    RETVAL AddKernelAttributePatchItems(
        const IGC::SOpenCLKernelInfo& annotations,
        Util::BinaryStream& membuf );

    RETVAL AddKernelArgumentPatchItems(
        const IGC::SOpenCLKernelInfo& annotations,
        Util::BinaryStream& membuf );

    RETVAL AddStringPatchItem(
        const std::string& str,
        Util::BinaryStream& membuf,
        uint32_t& bytesWritten ) const;

    inline DWORD GetSamplerStateSizeMultiplier(
        const SAMPLER_OBJECT_TYPE samplerType ) const;

    /// Returns TRUE if the samplers allow 3D images to be represented as arrays of 2D images.
    bool InlineSamplersAllow3DImageTransformation(
        const IGC::SOpenCLKernelInfo& annotations) const;

    // State
    G6HWC::SMediaHardwareCapabilities m_HWCaps;
    PLATFORM m_Platform;

    // Constants
    enum { DEFAULT_CONSTANT_BUFFER_INDEX = 0} ;
};

inline DWORD CGen8OpenCLStateProcessor::GetSamplerStateSizeMultiplier(
    const SAMPLER_OBJECT_TYPE samplerType ) const
{
    DWORD multiplier = 0;

    switch( samplerType )
    {
    case SAMPLER_OBJECT_TEXTURE:                    multiplier =   1; break;
    case SAMPLER_OBJECT_VME:                        multiplier =   8; break;
    case SAMPLER_OBJECT_SAMPLE_8X8_ERODE:           multiplier =   2; break;
    case SAMPLER_OBJECT_SAMPLE_8X8_DILATE:          multiplier =   2; break;
    case SAMPLER_OBJECT_SAMPLE_8X8_MINMAXFILTER:    multiplier =   2; break;
    case SAMPLER_OBJECT_SAMPLE_8X8_2DCONVOLVE:      multiplier = 128; break;
    case SAMPLER_OBJECT_SAMPLE_8X8_MINMAX:          multiplier =   0; break;
    case SAMPLER_OBJECT_SAMPLE_8X8_CENTROID:        multiplier =   0; break;
    case SAMPLER_OBJECT_SAMPLE_8X8_BOOL_CENTROID:   multiplier =   0; break;
    case SAMPLER_OBJECT_SAMPLE_8X8_BOOL_SUM:        multiplier =   0; break;
    default:
        IGC_ASSERT_MESSAGE(0, "Unknown sampler type");
        multiplier = 0;
        break;
    }

    return multiplier;
}

enum NumBarriersCode {
    None,
    B1,
    B2,
    B4,
    B8,
    B16,
    B24,
    B32
};

static inline uint32_t EncodeNumBarriers(uint32_t NumBarriers) {
    switch (NumBarriers)
    {
    case 0:
        return NumBarriersCode::None;
    case 1:
        return NumBarriersCode::B1;
    case 2:
        return NumBarriersCode::B2;
    case 4:
        return NumBarriersCode::B4;
    case 8:
        return NumBarriersCode::B8;
    case 16:
        return NumBarriersCode::B16;
    case 24:
        return NumBarriersCode::B24;
    case 32:
        return NumBarriersCode::B32;
    default:
        IGC_ASSERT_EXIT_MESSAGE(0, "invalid number of barriers is provided");
        return NumBarriersCode::None;
    }
}
}
