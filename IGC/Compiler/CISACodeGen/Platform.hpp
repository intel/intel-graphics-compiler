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
#include "inc/common/igfxfmid.h"
#include "Compiler/compiler_caps.h"
#include "Compiler/igc_workaround.h"
#include "common/Types.hpp"
#include "Probe/Assertion.h"
#include "common/igc_regkeys.hpp"


namespace IGC
{

class CPlatform
{
PLATFORM m_platformInfo;
SCompilerHwCaps m_caps;
WA_TABLE m_WaTable;
SKU_FEATURE_TABLE m_SkuTable;
GT_SYSTEM_INFO      m_GTSystemInfo;
OCLCaps m_OCLCaps;

public:
CPlatform(PLATFORM platform) {
    m_platformInfo = platform;
    m_GTSystemInfo = { 0 };
}
CPlatform() {}

void setOclCaps(OCLCaps& caps) { m_OCLCaps = caps; }
uint32_t getMaxOCLParameteSize() const {
  uint32_t limitFromFlag = IGC_GET_FLAG_VALUE(OverrideOCLMaxParamSize);
  return limitFromFlag ? limitFromFlag : m_OCLCaps.MaxParameterSize;
}
void OverrideRevId(unsigned short newRevId)
{
    m_platformInfo.usRevId = newRevId;
}

    WA_TABLE const& getWATable() const { return m_WaTable; }
SKU_FEATURE_TABLE const& getSkuTable() const { return m_SkuTable; }

bool hasPackedVertexAttr() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
bool hasScaledMessage() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
bool has8ByteA64ByteScatteredMessage() const { return m_platformInfo.eRenderCoreFamily == IGFX_GEN8_CORE; }

bool hasPredicatedBarriers() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
bool hasIEEEMinmaxBit() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
bool hasL1ReadOnlyCache() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE; }
// Gen10+ HW supports adding vertex start to vertex ID
bool hasVertexOffsetEnable() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE; }

// Gen10+ HW supports sending base instance, base vertex and draw index with
// VF, this is programmed using 3DSTATE_VF_SGVS2 command.
bool hasSGVS2Command() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE; }

// Sampler supports normalization of coordinates during sampling from
// rectangle textures.
bool supportsCoordinateNormalizationForRectangleTextures() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE; }

/// On some platforms ld sources order is: u lod v r
bool hasOldLdOrder() const { return m_platformInfo.eRenderCoreFamily <= IGFX_GEN8_CORE; }
bool supportSampleAndLd_lz() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
bool supportSamplerToRT() const {
        return (m_platformInfo.eProductFamily == IGFX_CHERRYVIEW) || (m_platformInfo.eRenderCoreFamily == IGFX_GEN9_CORE);
    }
    bool supportFP16() const {
        return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE) ||
            ((m_platformInfo.eRenderCoreFamily == IGFX_GEN8_CORE) && (m_platformInfo.eProductFamily == IGFX_CHERRYVIEW));
    }
bool supportFP16Rounding() const { return false; }
bool supportSamplerFp16Input() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE; }
bool supportPooledEU() const { return m_SkuTable.FtrPooledEuEnabled != 0; }
bool supportSplitSend() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
bool supportSendInstShootdown() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE; }
bool supportHSEightPatchDispatch() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
bool supportSingleInstanceVertexShader() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE; }
bool supportDSDualPatchDispatch() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
bool needsHSBarrierIDWorkaround() const { return m_platformInfo.eRenderCoreFamily <= IGFX_GEN10_CORE; }
bool supportBindless() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
bool supportsBindlessSamplers() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE; }

bool SupportSurfaceInfoMessage() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
bool SupportHDCUnormFormats() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE; }
    bool localMemFenceSupress() const {
        return m_platformInfo.eRenderCoreFamily <= IGFX_GEN9_CORE ||
            IGC_IS_FLAG_ENABLED(DisbleLocalFences);
    }
bool psSimd32SkipStallHeuristic() const { return m_caps.KernelHwCaps.EUThreadsPerEU == 6; }
bool enablePSsimd32() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
bool supportDisableMidThreadPreemptionSwitch() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE; }

bool needSWStencil() const
{
        return (m_platformInfo.eRenderCoreFamily == IGFX_GEN9_CORE && IGC_IS_FLAG_ENABLED(EnableSoftwareStencil));
}
bool supportMSAARateInPayload() const
{
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE;
}

bool support16BitImmSrcForMad() const {
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE);
}
GFXCORE_FAMILY GetPlatformFamily() const { return m_platformInfo.eRenderCoreFamily; }
const PLATFORM& getPlatformInfo() const { return m_platformInfo; }
void SetCaps(const SCompilerHwCaps& caps) { m_caps = caps; }
void SetWATable(const WA_TABLE& waTable) { m_WaTable = waTable; }
void SetSkuTable(const SKU_FEATURE_TABLE& skuTable) { m_SkuTable = skuTable; }
void SetGTSystemInfo(const SUscGTSystemInfo gtSystemInfo) {
    m_GTSystemInfo.EUCount = gtSystemInfo.EUCount;
    m_GTSystemInfo.ThreadCount = gtSystemInfo.ThreadCount;
    m_GTSystemInfo.SliceCount = gtSystemInfo.SliceCount;
    m_GTSystemInfo.SubSliceCount = gtSystemInfo.SubSliceCount;
    m_GTSystemInfo.TotalPsThreadsWindowerRange = gtSystemInfo.TotalPsThreadsWindowerRange;
    m_GTSystemInfo.TotalVsThreads = gtSystemInfo.TotalVsThreads;
    m_GTSystemInfo.TotalVsThreads_Pocs = gtSystemInfo.TotalVsThreads_Pocs;
    m_GTSystemInfo.TotalDsThreads = gtSystemInfo.TotalDsThreads;
    m_GTSystemInfo.TotalGsThreads = gtSystemInfo.TotalGsThreads;
    m_GTSystemInfo.TotalHsThreads = gtSystemInfo.TotalHsThreads;
    m_GTSystemInfo.MaxEuPerSubSlice = gtSystemInfo.MaxEuPerSubSlice;
    m_GTSystemInfo.EuCountPerPoolMax = gtSystemInfo.EuCountPerPoolMax;
    m_GTSystemInfo.EuCountPerPoolMin = gtSystemInfo.EuCountPerPoolMin;
    m_GTSystemInfo.MaxSlicesSupported = gtSystemInfo.MaxSlicesSupported;
    m_GTSystemInfo.MaxSubSlicesSupported = gtSystemInfo.MaxSubSlicesSupported;
    m_GTSystemInfo.IsDynamicallyPopulated = gtSystemInfo.IsDynamicallyPopulated;
    m_GTSystemInfo.CsrSizeInMb = gtSystemInfo.CsrSizeInMb;
}

    void SetGTSystemInfo(const GT_SYSTEM_INFO& gtSystemInfo) {
    m_GTSystemInfo = gtSystemInfo;
}

GT_SYSTEM_INFO GetGTSystemInfo() const { return m_GTSystemInfo; }

    unsigned int getMaxPixelShaderThreads() const {
        return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE ?
            m_caps.PixelShaderThreadsWindowerRange - 1 : m_caps.PixelShaderThreadsWindowerRange - 2;
    }
bool supportGPGPUMidThreadPreemption() const {
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE &&
        m_platformInfo.eRenderCoreFamily <= IGFX_GEN11LP_CORE;
}
bool supportFtrWddm2Svm() const { return m_SkuTable.FtrWddm2Svm != 0; }
bool supportStructuredAsRaw() const {
        return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE;
    }
bool supportSamplerCacheResinfo() const { return m_platformInfo.eRenderCoreFamily == IGFX_GEN8_CORE; }

unsigned int getMaxVertexShaderThreads(const bool isPositionOnlyShader) const
{
    const unsigned int maxVertexShaderThreads = isPositionOnlyShader ? m_caps.VertexShaderThreadsPosh : m_caps.VertexShaderThreads;
    return maxVertexShaderThreads - 1;
}
unsigned int getMaxGeometryShaderThreads() const { return m_caps.GeometryShaderThreads - 1; }
unsigned int getMaxHullShaderThreads() const { return m_caps.HullShaderThreads - 1; }
unsigned int getMaxDomainShaderThreads() const { return m_caps.DomainShaderThreads - 1; }
unsigned int getMaxGPGPUShaderThreads() const { return m_caps.MediaShaderThreads - 1; }
unsigned int getKernelPointerAlignSize() const { return m_caps.KernelHwCaps.KernelPointerAlignSize; }
unsigned int getSharedLocalMemoryBlockSize() const { return m_caps.SharedLocalMemoryBlockSize; }
unsigned int getMaxNumberThreadPerSubslice() const
{
    //total number of threads per subslice
        if (m_caps.KernelHwCaps.SubSliceCount != 0)
        return m_caps.KernelHwCaps.ThreadCount / m_caps.KernelHwCaps.SubSliceCount;
    return 0;
}
unsigned int getMaxNumberThreadPerWorkgroupPooledMax() const
{
    return m_caps.KernelHwCaps.EUCountPerPoolMax * m_caps.KernelHwCaps.EUThreadsPerEU;
}
unsigned int getFFTIDBitMask() const {
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE) ? 0x3FF : 0x1FF;
}
unsigned int getBarrierCountBits(unsigned int count) const
{
    // Returns barrier count field + enable for barrier message
    if (m_platformInfo.eRenderCoreFamily <= IGFX_GEN10_CORE)
    {
        // up to Gen9 barrier count is in bits 14:9, enable is bit 15
        return (count << 9) | (1 << 15);
    }
    else
    {
        // for Gen10+ barrier count is in bits 14:8, enable is bit 15
        return (count << 8) | (1 << 15);
    }
}

bool supportsDrawParametersSGVs() const
{
    // Gen10+, 3DSTATE_VF_SGVS_2
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE;
}

bool hasPSDBottleneck() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE; }

bool supportsHardwareResourceStreamer() const
{
    return m_platformInfo.eRenderCoreFamily < IGFX_GEN11_CORE;
}
bool AOComputeShadersSIMD32Mode() const
{
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE);
}
unsigned int getHullShaderThreadInstanceIdBitFieldPosition() const
{
    // HS thread receives instance ID in R0.2 bits 22:16 for Gen10+ and bits 23:17 for older Gens
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE) ? 16 : 17;
}
bool supportsBinaryAtomicCounterMessage() const
{
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE;
}
bool supportSLMBlockMessage() const
{
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE);
}

bool hasSLMFence() const
{
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE);
}

bool supportRotateInstruction() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE; }
bool supportLRPInstruction() const { return m_platformInfo.eRenderCoreFamily < IGFX_GEN11_CORE; }
bool support16bitMSAAPayload() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE; }
bool supportTwoStackTSG() const
{
    //Will need check for specific skus where TwoStackTSG is enabled
    //Not all skus have it enabled
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE);
}
bool enableBlendToDiscardAndFill() const
{
    return (m_platformInfo.eRenderCoreFamily < IGFX_GEN11_CORE);
}
bool HSUsesHWBarriers() const
{
    // HS HW barriers work correctly since ICL platform.
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE);
}

bool enableVertexReorderingPhase2() const
{
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE);
}

unsigned int GetLogBindlessSamplerSize() const
{
    // Samplers are 16 bytes
    return 4;
}

bool SupportCPS() const
{
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE);
}

bool supportsThreadCombining() const
{
    return !(!m_WaTable.WaEnablePooledEuFor2x6 &&
        m_platformInfo.eProductFamily == IGFX_BROXTON &&
        m_GTSystemInfo.SubSliceCount == 2);
}

bool enableMaxWorkGroupSizeCalculation() const
{
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE) &&
        IGC_IS_FLAG_ENABLED(EnableMaxWGSizeCalculation);
}

bool has8DWA64ScatteredMessage() const { return m_platformInfo.eRenderCoreFamily < IGFX_GEN12_CORE; }

bool flushL3ForTypedMemory() const
{
    return m_platformInfo.eRenderCoreFamily <= IGFX_GEN11_CORE;
}

bool supportsStencil(SIMDMode simdMode) const
{
    return getMinDispatchMode() == SIMDMode::SIMD16 ? true : simdMode == SIMDMode::SIMD8;
}

bool hasFDIV() const {
    if (IGC_IS_FLAG_ENABLED(DisableFDIV))
    return false;
    return (m_platformInfo.eRenderCoreFamily < IGFX_GEN12_CORE);
}

bool doIntegerMad() const
{
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE && m_platformInfo.eProductFamily != IGFX_DG1 &&
        IGC_IS_FLAG_ENABLED(EnableIntegerMad);
}

bool isDG1() const
{
    return m_platformInfo.eProductFamily == IGFX_DG1;
}

bool supportsSIMD16TypedRW() const
{
    return false;
}

bool singleThreadBasedInstScheduling() const
{
    return true;
}

bool HDCCoalesceAtomicCounterAccess() const
{
    return IGC_IS_FLAG_DISABLED(ForceSWCoalescingOfAtomicCounter);
}

//all the platforms which DONOT support 64 bit int operations
bool hasNoInt64Inst() const {
    return m_platformInfo.eProductFamily == IGFX_ICELAKE_LP ||
        m_platformInfo.eProductFamily == IGFX_LAKEFIELD ||
        m_platformInfo.eProductFamily == IGFX_ELKHARTLAKE ||
        m_platformInfo.eProductFamily == IGFX_JASPERLAKE ||
        m_platformInfo.eProductFamily == IGFX_TIGERLAKE_LP ||
        m_platformInfo.eProductFamily == IGFX_DG1;
}

//all the platforms which DONOT support 64 bit float operations
bool hasNoFP64Inst() const {
    return m_platformInfo.eProductFamily == IGFX_ICELAKE_LP ||
        m_platformInfo.eProductFamily == IGFX_LAKEFIELD ||
        m_platformInfo.eProductFamily == IGFX_ELKHARTLAKE ||
        m_platformInfo.eProductFamily == IGFX_JASPERLAKE ||
        m_platformInfo.eProductFamily == IGFX_TIGERLAKE_LP ||
        m_platformInfo.eProductFamily == IGFX_DG1;
}

//all the platforms which do not support 64 bit operations and
//needs int64 emulation support. Except also for BXT where
//64-bit inst has much lower throughput compared to SKL.
//Emulating it improves performance on some benchmarks and
//won't have impact on the overall performance.
bool need64BitEmulation() const {
    return (m_platformInfo.eProductFamily == IGFX_GEMINILAKE ||
        m_platformInfo.eProductFamily == IGFX_BROXTON ||
        hasNoInt64Inst());
}


//all the platforms which have correctly rounded macros (INVM, RSQRTM, MADM)
bool hasCorrectlyRoundedMacros() const {
    return m_platformInfo.eProductFamily != IGFX_ICELAKE_LP &&
        m_platformInfo.eProductFamily != IGFX_LAKEFIELD &&
        m_platformInfo.eProductFamily != IGFX_JASPERLAKE &&
        m_platformInfo.eProductFamily != IGFX_TIGERLAKE_LP &&
        m_platformInfo.eProductFamily != IGFX_DG1;
}

bool hasHWDp4AddSupport() const {
    return m_platformInfo.eProductFamily == IGFX_TIGERLAKE_LP;
}
bool useOnlyEightPatchDispatchHS() const { return false; }
bool hasFusedEU() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE; }
bool supports256GRFPerThread() const { return false; }
bool supportMixMode() const {
    return IGC_IS_FLAG_ENABLED(ForceMixMode) ||
        (IGC_IS_FLAG_DISABLED(DisableMixMode) &&
        (m_platformInfo.eProductFamily == IGFX_CHERRYVIEW ||
            m_platformInfo.eRenderCoreFamily == IGFX_GEN9_CORE ||
            m_platformInfo.eRenderCoreFamily == IGFX_GEN10_CORE));
}
bool DSPrimitiveIDPayloadPhaseCanBeSkipped() const { return false; }
bool hasAtomicPreDec() const { return true; }
bool needsHeaderForAtomicCounter() const { return false; }
bool doScalar64bScan() const
{
    return false;
}
bool useScratchSpaceForOCL() const
{
    return IGC_IS_FLAG_ENABLED(EnableOCLScratchPrivateMemory);
}

uint32_t getGRFSize() const
{
    return 32;
}

// If true then screen space coordinates for upper-left vertex of a triangle
// being rasterized are delivered together with source depth or W deltas.
bool hasStartCoordinatesDeliveredWithDeltas() const
{
    return false;
}

uint32_t maxPerThreadScratchSpace() const
{
    return 0x200000;
}

bool supportByteALUOperation() const
{
    return true;
}

bool NeedsHDCFenceBeforeEOTInPixelShader() const
{
    return false;
}

bool canFuseTypedWrite() const
{
    return IGC_IS_FLAG_ENABLED(FuseTypedWrite);
}

unsigned int getMaxNumberHWThreadForEachWG() const
{
    return getMaxNumberThreadPerSubslice();
}

// max block size for legacy OWord block messages
uint32_t getMaxBlockMsgSize(bool isSLM) const
{
    return 128;
}

SIMDMode getMinDispatchMode() const
{
    return SIMDMode::SIMD8;
}

unsigned getAccChNumUD() const
{
    return 8;
}
// ***** Below go accessor methods for testing WA data from WA_TABLE *****

bool WaDoNotPushConstantsForAllPulledGSTopologies() const
{
    return (m_platformInfo.eProductFamily == IGFX_BROADWELL) ||
        m_WaTable.WaDoNotPushConstantsForAllPulledGSTopologies != 0;
}

bool WaForceMinMaxGSThreadCount() const
{
    return m_WaTable.WaForceMinMaxGSThreadCount != 0;
}

bool WaOCLEnableFMaxFMinPlusZero() const
{
    return m_WaTable.WaOCLEnableFMaxFMinPlusZero != 0;
}

bool WaDisableSendsSrc0DstOverlap() const
{
    return m_WaTable.WaDisableSendsSrc0DstOverlap != 0;
}

bool WaDisableEuBypass() const
{
        return (m_WaTable.WaDisableEuBypassOnSimd16Float32 != 0);
}

bool WaDisableDSDualPatchMode() const
{
    return m_WaTable.WaDisableDSDualPatchMode == 0;
}

bool WaDispatchGRFHWIssueInGSAndHSUnit() const
{
    return m_WaTable.WaDispatchGRFHWIssueInGSAndHSUnit != 0;
}

bool WaSamplerResponseLengthMustBeGreaterThan1() const
{
    return m_WaTable.WaSamplerResponseLengthMustBeGreaterThan1 != 0;
}

bool WaDisableDSPushConstantsInFusedDownModeWithOnlyTwoSubslices() const
{
    return ((m_WaTable.WaDisableDSPushConstantsInFusedDownModeWithOnlyTwoSubslices) &&
            (m_GTSystemInfo.IsDynamicallyPopulated && m_GTSystemInfo.SubSliceCount == 2));
}

bool WaDisableVSPushConstantsInFusedDownModeWithOnlyTwoSubslices() const
{
    return ((m_WaTable.WaDisableVSPushConstantsInFusedDownModeWithOnlyTwoSubslices) &&
            (m_GTSystemInfo.IsDynamicallyPopulated && m_GTSystemInfo.SubSliceCount == 2));
}

bool WaForceCB0ToBeZeroWhenSendingPC() const
{
    return m_WaTable.WaForceCB0ToBeZeroWhenSendingPC != 0;
}

bool WaConservativeRasterization() const
{
    return (m_WaTable.WaConservativeRasterization != 0 &&
        IGC_IS_FLAG_ENABLED(ApplyConservativeRastWAHeader));
}

bool WaReturnZeroforRTReadOutsidePrimitive() const
{
    return m_WaTable.WaReturnZeroforRTReadOutsidePrimitive != 0;
}

bool WaFixInnerCoverageWithSampleMask() const
{
    return m_WaTable.Wa_220856683 != 0;
}

bool WaForceDSToWriteURB() const
{
    return m_WaTable.Wa_1805992985 != 0;
}

bool WaOverwriteFFID() const
{
    return m_WaTable.Wa_1409460247 != 0;
}

bool alignBindlessSampler() const
{
    return true;
}

bool WaDisableSendSrcDstOverlap() const
{
    return (!IGC_IS_FLAG_ENABLED(DisableSendSrcDstOverlapWA)) &&
        (m_SkuTable.FtrWddm2Svm != 0 || m_platformInfo.eRenderCoreFamily == IGFX_GEN10_CORE ||
            m_platformInfo.eRenderCoreFamily == IGFX_GEN11_CORE);

}

bool WaInsertHDCFenceBeforeEOTWhenSparseAliasedResources() const
{
    return m_WaTable.Wa_1807084924 != 0;
}

bool WaEnableA64WA() const
{
    if (IGC_IS_FLAG_ENABLED(EnableA64WA) && m_WaTable.Wa_14010595310) {
        return true;
    }
    return false;
}

//Only enable this WA for TGLLP+ because, in pre TGLLP projects, smov was replaced with two instructions which caused performance penalty.
bool enableMultiGRFAccessWA() const
{
    return (m_platformInfo.eProductFamily >= IGFX_TIGERLAKE_LP);
}

const SCompilerHwCaps& GetCaps() { return m_caps; }
};

}//namespace IGC
