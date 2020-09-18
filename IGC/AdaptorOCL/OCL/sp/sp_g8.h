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

#include <vector>

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

      virtual ~IProgramContext () {}
    };

    explicit CGen8OpenCLStateProcessor(PLATFORM platform, const IProgramContext& Ctx);
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
        const char*  rawDebugDataVISA,
        unsigned int rawDebugDataVISASize,
        const char*  rawDebugDataGenISA,
        unsigned int rawDebugDataGenISASize,
        const std::string& kernelName,
        Util::BinaryStream& kernelDebugData);

    const IProgramContext& m_Context;
    std::string m_oclStateDebugMessagePrintOut;

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
        const bool gtpinEnabled,
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
        const bool gtpinEnabled,
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

#if defined(_DEBUG) || defined(_INTERNAL) || defined(_RELEASE_INTERNAL)
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

}
