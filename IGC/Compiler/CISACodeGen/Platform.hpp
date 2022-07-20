/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "inc/common/igfxfmid.h"
#include "Compiler/compiler_caps.h"
#include "Compiler/igc_workaround.h"
#include "Compiler/API/ShaderTypesEnum.h"
#include "common/Types.hpp"
#include "Probe/Assertion.h"
#include "common/igc_regkeys.hpp"

#include "../../../skuwa/iacm_g10_rev_id.h"
#include "../../../skuwa/iacm_g11_rev_id.h"
#include "../../../skuwa/iacm_g12_rev_id.h"

namespace IGC
{

class CPlatform
{
    PLATFORM m_platformInfo = {};
    SCompilerHwCaps m_caps = {};
    WA_TABLE m_WaTable = {};
    SKU_FEATURE_TABLE m_SkuTable = {};
    GT_SYSTEM_INFO m_GTSystemInfo = {};
    OCLCaps m_OCLCaps = {};

public:
    CPlatform() {}

    CPlatform(const PLATFORM& platform)
    {
        m_platformInfo = platform;
    }

public:
void setOclCaps(OCLCaps& caps) { m_OCLCaps = caps; }
uint32_t getMaxOCLParameteSize() const {
    uint32_t limitFromFlag = IGC_GET_FLAG_VALUE(OverrideOCLMaxParamSize);
    return limitFromFlag ? limitFromFlag : m_OCLCaps.MaxParameterSize;
}
void OverrideRevId(unsigned short newRevId)
{
    m_platformInfo.usRevId = newRevId;
}

void OverrideDeviceId(unsigned short newDeviceID)
{
    m_platformInfo.usDeviceID = newDeviceID;
}


void OverrideProductFamily(unsigned int productID)
{
    PRODUCT_FAMILY eProd = static_cast<PRODUCT_FAMILY>(productID);
    if(eProd > IGFX_UNKNOWN && eProd < IGFX_MAX_PRODUCT)
        m_platformInfo.eProductFamily = (PRODUCT_FAMILY)productID;
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

bool supportSimd32PerPixelPSWithNumSamples16() const
{
    return false;
}


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

// This function checks if product is child of another product
bool isProductChildOf(PRODUCT_FAMILY product) const
{
    if (product == IGFX_PVC)
        return isCoreChildOf(IGFX_XE_HPC_CORE);
    return m_platformInfo.eProductFamily >= product;
}

// This function checks if core is child of another core
bool isCoreChildOf(GFXCORE_FAMILY core) const
{
    return m_platformInfo.eRenderCoreFamily >= core;
}

bool supports8DWLSCMessage() const {
    return (SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_B0) && m_platformInfo.eProductFamily == IGFX_DG2)
            || GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID)
            || GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID)
            || isCoreChildOf(IGFX_XE_HPC_CORE);
}

PRODUCT_FAMILY GetProductFamily() const { return m_platformInfo.eProductFamily; }
unsigned short GetDeviceId() const { return m_platformInfo.usDeviceID; }
unsigned short GetRevId() const { return m_platformInfo.usRevId; }
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
    m_GTSystemInfo.SLMSizeInKb = gtSystemInfo.SLMSizeInKb;
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

bool NeedResetA0forVxHA0() const
{
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE);
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
    return (!(!m_WaTable.WaEnablePooledEuFor2x6 &&
        m_platformInfo.eProductFamily == IGFX_BROXTON &&
        m_GTSystemInfo.SubSliceCount == 2))
        && (m_platformInfo.eRenderCoreFamily < IGFX_GEN12_CORE);
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

bool simplePushIsFasterThanGather() const
{
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE;
}

bool singleThreadBasedInstScheduling() const
{
    return m_platformInfo.eRenderCoreFamily < IGFX_GEN12_CORE;
}

//all the platforms which do not support 64 bit operations and
//needs int64 emulation support. Except also for BXT where
//64-bit inst has much lower throughput compared to SKL.
//Emulating it improves performance on some benchmarks and
//won't have impact on the overall performance.
bool need64BitEmulation() const {
    return m_platformInfo.eProductFamily == IGFX_GEMINILAKE ||
        m_platformInfo.eProductFamily == IGFX_BROXTON ||
        hasNoInt64Inst();
}

bool HDCCoalesceSLMAtomicINCWithNoReturn() const
{
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE;
}

bool HDCCoalesceAtomicCounterAccess() const
{
    return (m_platformInfo.eRenderCoreFamily < IGFX_GEN12_CORE) && IGC_IS_FLAG_DISABLED(ForceSWCoalescingOfAtomicCounter);
}

bool supportsMCSNonCompressedFix() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE; }

bool hasHWDp4AddSupport() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE; }

bool useOnlyEightPatchDispatchHS() const
{
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE);
}

bool supportsPrimitiveReplication() const
{
    return ((m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE) ||
            (m_platformInfo.eRenderCoreFamily == IGFX_GEN11_CORE && m_platformInfo.eProductFamily == IGFX_ICELAKE));
}

// If true then screen space coordinates for upper-left vertex of a triangle
// being rasterized are delivered together with source depth or W deltas.
bool hasStartCoordinatesDeliveredWithDeltas() const
{
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE;
}

bool disableStaticVertexCount() const
{
    return m_WaTable.Wa_14012504847 != 0 || IGC_IS_FLAG_ENABLED(ForceStaticToDynamic);
}

bool hasSamplerSupport() const
{
    return (m_platformInfo.eRenderCoreFamily != IGFX_XE_HPC_CORE) ||
        (IGC_IS_FLAG_ENABLED(EnableSamplerSupport)); // flag for IGFX_PVC
}

uint32_t getMinPushConstantBufferAlignment() const
{
    return 8; // DWORDs
}

// Returns the default limit of pushed constant data in GRFs. This value limits
// the amount of constant buffer data promoted to registers.
uint32_t getBlockPushConstantGRFThreshold() const
{
    constexpr uint32_t defaultThreshold = 31;
    constexpr uint32_t gen9GT2Threshold = 15;

    const GTTYPE gt = m_platformInfo.eGTType;
    switch (m_platformInfo.eProductFamily)
    {
    case IGFX_COFFEELAKE:
    case IGFX_SKYLAKE:
    case IGFX_KABYLAKE:
        return (gt == GTTYPE_GT2) ? gen9GT2Threshold : defaultThreshold;
    default:
        return defaultThreshold;
    }
}

bool hasDualSubSlices() const
{
    bool hasDualSS = m_platformInfo.eRenderCoreFamily == IGFX_GEN12_CORE ||
        m_platformInfo.eRenderCoreFamily == IGFX_GEN12LP_CORE;
    if (m_platformInfo.eRenderCoreFamily == IGFX_XE_HPG_CORE)
    {
        hasDualSS = true;
    }
    return hasDualSS;
}

unsigned getSlmSizePerSsOrDss() const
{
    // GTSysInfo sets SLMSize only for gen12+
    unsigned slmSizePerSsOrDss = 65536;
    if (hasDualSubSlices())
    {
        if (GetGTSystemInfo().DualSubSliceCount)
        {
            slmSizePerSsOrDss = GetGTSystemInfo().SLMSizeInKb / GetGTSystemInfo().DualSubSliceCount * 1024;
        }
        else
        {
            slmSizePerSsOrDss = 131072;
        }
    }
    return slmSizePerSsOrDss;
}

bool canForcePrivateToGlobal() const
{
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE && IGC_IS_FLAG_ENABLED(ForcePrivateMemoryToGlobalOnGeneric);
}

bool getHWTIDFromSR0() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

bool supportAdd3Instruction() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

bool supportBfnInstruction() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

bool supportDpasInstruction() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

bool hasPackedRestrictedFloatVector() const
{
    return true;
}

bool supportLoadThreadPayloadForCompute() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

bool support16BitAtomics() const
{
    return m_platformInfo.eProductFamily >= IGFX_GEN12_CORE;
}

bool Enable32BitIntDivRemEmu() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

bool support16DWURBWrite() const
{
    return IGC_IS_FLAG_ENABLED(Enable16DWURBWrite) && isProductChildOf(IGFX_XE_HP_SDV);
}

bool hasScratchSurface() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

bool hasAtomicPreDec() const
{
    return !isProductChildOf(IGFX_XE_HP_SDV);
}

bool support26BitBSOFormat() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

bool needsHeaderForAtomicCounter() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

bool doScalar64bScan() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

bool hasHWLocalThreadID() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

unsigned int getOfflineCompilerMaxWorkGroupSize() const
{
    if (isProductChildOf(IGFX_XE_HP_SDV))
        return 1024;
    return 448;
}

bool hasFP16AtomicMinMax() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

bool hasFP32GlobalAtomicAdd() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

bool has16OWSLMBlockRW() const
{
    return IGC_IS_FLAG_ENABLED(Enable16OWSLMBlockRW) && isProductChildOf(IGFX_XE_HP_SDV);
}

bool hasLargeMaxConstantBufferSize() const
{
    return IGC_IS_FLAG_DISABLED(Force32bitConstantGEPLowering) &&
        m_platformInfo.eProductFamily == IGFX_PVC;
}

bool supportInlineData() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}

// TODO: temporary solution, remove this once it's not needed
bool supportInlineDataOCL() const
{
    if (m_platformInfo.eRenderCoreFamily == IGFX_XE_HPC_CORE)
    {
        return false;
    }
    return isProductChildOf(IGFX_XE_HP_SDV);
}

bool supportsAutoGRFSelection() const
{
    return isProductChildOf(IGFX_PVC) || m_platformInfo.eProductFamily == IGFX_XE_HP_SDV || (m_platformInfo.eProductFamily == IGFX_DG2 && IGC_IS_FLAG_ENABLED(ForceSupportsAutoGRFSelection));
}

float adjustedSpillThreshold() const
{
    return isProductChildOf(IGFX_DG2) ? 9.0f : 12.0f;
}

bool hasLSC() const
{
    return IGC_IS_FLAG_DISABLED(ForceNoLSC) && !WaEnableLSCBackupMode() && isProductChildOf(IGFX_DG2);
}

bool WaEnableLSCBackupMode() const
{
    return (m_WaTable.Wa_14010198302 != 0);
}

bool supportQWRotateInstructions() const
{
    return m_platformInfo.eRenderCoreFamily == IGFX_XE_HPC_CORE && IGC_IS_FLAG_ENABLED(EnableQWRotateInstructions);
}

bool loosenSimd32occu() const
{
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE && m_platformInfo.eProductFamily != IGFX_DG2);
}

// Local memory here refers to memory on the device- e.g. HBM for PVC.
bool hasLocalMemory() const
{
    return m_SkuTable.FtrLocalMemory != 0;
}

bool enableImmConstantOpt() const
{
    return !isProductChildOf(IGFX_XE_HP_SDV);
}

bool supportsTier2VRS() const
{
    return m_platformInfo.eProductFamily >= IGFX_DG2;
}

bool supportsSIMD16TypedRW() const
{
    return isCoreChildOf(IGFX_XE_HPC_CORE);
}

bool supportHWGenerateTID() const
{
    return IGC_IS_FLAG_ENABLED(EnableHWGenerateThreadID) && isProductChildOf(IGFX_DG2);
}

bool hasHalfSIMDLSC() const
{
    return (m_platformInfo.eProductFamily == IGFX_DG2 && SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_B0)) ||
        GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID) ||
        GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID) ||
        // false for PVC XL A0 RevID==0x0, true from PVC XT A0 RevID==0x3==REVISION_B
        (m_platformInfo.eProductFamily == IGFX_PVC && m_platformInfo.usRevId >= REVISION_B);
}

bool NeedsLSCFenceUGMBeforeEOT() const
{
    return getWATable().Wa_22013689345 != 0;
}

bool hasPartialInt64Support() const
{
    // false for PVC XL A0 RevID==0x0, true from PVC XT A0 RevID==0x3==REVISION_B
    return (m_platformInfo.eProductFamily == IGFX_PVC && m_platformInfo.usRevId >= REVISION_B) ||
        IGC_IS_FLAG_ENABLED(ForcePartialInt64);
}

bool hasNoInt64AddInst() const
{
    // to be changed for PVC-XT with Add Int64 support;
    return hasNoFullI64Support() && !hasQWAddSupport();
}

bool hasQWAddSupport() const
{
    return (m_platformInfo.eProductFamily == IGFX_PVC &&
        ((m_platformInfo.usRevId >= REVISION_D && IGC_IS_FLAG_ENABLED(EnableQWAddSupport))  // true from PVC XT B0 RevID==0x5==REVISION_D
        || IGC_IS_FLAG_ENABLED(ForceQWAddSupport))); // back door way to enable feature along with ForcePartialInt64 - needed to perform the experiments on PVC-A
}

bool hasExecSize16DPAS() const
{
    return isCoreChildOf(IGFX_XE_HPC_CORE);
}

bool LSCSimd1NeedFullPayload() const
{
    // in PVC XL A0 RevID=0x0, SIMD1 reads/writes need full payloads
    // this causes chaos for vISA (would need 4REG alignment)
    // and to make extra moves to enable the payload
    // PVC XT A0 RevID==0x3==REVISION_B gets this feature
    return (m_platformInfo.eProductFamily == IGFX_PVC &&
        m_platformInfo.usRevId < REVISION_B);
}

bool hasNoFullI64Support() const
{
    return (hasNoInt64Inst() || hasPartialInt64Support());
}

bool WaPredicatedStackIDRelease() const
{
    return m_WaTable.Wa_22014559856 &&
           IGC_IS_FLAG_DISABLED(DisablePredicatedStackIDRelease);
}

// This returns the current maximum size that we recommend for performance.
// SIMD32 is still allowed and we may relax this in the future.
SIMDMode getMaxRayQuerySIMDSize() const
{
    if (m_platformInfo.eProductFamily <= IGFX_PVC)
    {
        return SIMDMode::SIMD16;
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "Change code here to support new platform!");
        return SIMDMode::UNKNOWN;
    }
}

SIMDMode getPreferredRayTracingSIMDSize() const
{
    return isCoreChildOf(IGFX_XE_HPC_CORE) ? SIMDMode::SIMD16 : SIMDMode::SIMD8;
}

bool supportRayTracing() const
{
    return isProductChildOf(IGFX_DG2);
}

bool isValidNumThreads(uint32_t numThreadsPerEU) const
{
    return numThreadsPerEU == 4 || numThreadsPerEU == 8;
}

bool supports3DAndCubeSampleD() const
{
    // Make sure to match hasSupportForSampleDOnCubeTextures LLVM3DBuilder\BuiltinsFrontend.hpp
    return (
        m_platformInfo.eProductFamily != IGFX_XE_HP_SDV &&
        m_platformInfo.eProductFamily != IGFX_DG2
        ) ||
        IGC_IS_FLAG_DISABLED(EnableSampleDEmulation);
}

bool LSCEnabled(SIMDMode m = SIMDMode::UNKNOWN) const
{
    if (IGC_IS_FLAG_ENABLED(EnableLSC))
        return true;
    if (hasLSC())
    {
        switch (m_platformInfo.eProductFamily)
        {
        case IGFX_PVC:
            if (m == SIMDMode::UNKNOWN)
            {
                // Must generate LSC from PVC XT A0 RevID==0x3==REVISION_B (not include XL A0)
                return m_platformInfo.usRevId >= REVISION_B;
            }
            return (m == SIMDMode::SIMD16 && m_platformInfo.usRevId > REVISION_A0)
                || m == SIMDMode::SIMD32;
        case IGFX_DG2:
            if (m == SIMDMode::UNKNOWN)
            {
                // Must generate LSC after A0 (not include A0)
                return (SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_B0) ||
                        GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID) ||
                        GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID)
                        );
            }
            return ((SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_B0) ||
                     GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID) ||
                     GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID)
                ) && m == SIMDMode::SIMD8) ||
                m == SIMDMode::SIMD16;
        default:
            return true;
        }
    }
    return false;
}

// Max LSC message block size is 8GRF
uint32_t getMaxLSCBlockMsgSize(bool isD64 = true) const
{
    return (isD64 ? 8 : 4) * getGRFSize();
}

bool hasURBFence() const
{
    return m_platformInfo.eProductFamily == IGFX_DG2;
}

bool hasMultiTile() const
{
    //FIXME: what to do for AOT compile?
    return m_GTSystemInfo.MultiTileArchInfo.TileCount > 1;
}

// UGM LSC fence with GPU scope triggers L3 flush
bool hasL3FlushOnGPUScopeInvalidate() const
{
    return m_platformInfo.eProductFamily == IGFX_DG2;
}

bool L3CacheCoherentCrossTiles() const {
    return isCoreChildOf(IGFX_XE_HPC_CORE);
}

bool AllowFenceOpt() const
{
    return ((m_platformInfo.eProductFamily == IGFX_DG2
        || m_platformInfo.eProductFamily == IGFX_PVC) &&
        IGC_IS_FLAG_ENABLED(EnablePlatformFenceOpt));
}

//if RayTracing fence WA for LSCBackupMode (DG2.A0 only actually) is enabled.
//In case of RayQuery of DG2.A0, we have to flush L1 after RTWrite and before shader read.
//The reason is, for RT writes, write - completion must guarantee that
//subsequent read with LSC bypass can read.This issue can be present
//even in async mode too but in sync mode read happens rather quickly
//compared to BTD dispatched thread.
bool RTFenceWAforBkModeEnabled() const
{
    return WaEnableLSCBackupMode();
}

SIMDMode getMinDispatchMode() const
{
    return isCoreChildOf(IGFX_XE_HPC_CORE) ? SIMDMode::SIMD16 : SIMDMode::SIMD8;
}

bool hasLSCTypedMessage() const
{
    return isCoreChildOf(IGFX_XE_HPC_CORE);
}

SIMDMode getMaxLSCTypedMessageSize() const
{
    switch (m_platformInfo.eRenderCoreFamily)
    {
    case IGFX_XE_HPG_CORE:
        return SIMDMode::SIMD8;
    case IGFX_XE_HPC_CORE:
        return SIMDMode::SIMD16;
    default:
            return SIMDMode::SIMD16;
    }
}

unsigned getAccChNumUD() const
{
    return isCoreChildOf(IGFX_XE_HPC_CORE) ? 16 : 8;
}

bool hasInt64SLMAtomicCAS() const
{
    // false for PVC XL A0 RevID==0x0, true from PVC XT A0 RevID==0x3==REVISION_B
    return m_platformInfo.eProductFamily == IGFX_PVC && m_platformInfo.usRevId >= REVISION_B;

}

bool hasFP64GlobalAtomicAdd() const
{
    return m_platformInfo.eProductFamily == IGFX_PVC && m_platformInfo.usRevId > REVISION_A0;
}

bool supports16BitLdMcs() const
{
    return isProductChildOf(IGFX_XE_HP_SDV) && IGC_IS_FLAG_ENABLED(Enable16BitLDMCS);
}

bool supportsGather4PO() const
{
    return !isProductChildOf(IGFX_XE_HP_SDV);
}

// Typed read supports all renderable image formats, no image data conversion is
// required.
bool typedReadSupportsAllRenderableFormats() const
{
    bool isChildOfDG2B0 = SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_B0);
    bool isChildOfDG2C0 = SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_C0);
    bool isDG2G11Config = GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID);
    bool isDG2G12Config = GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID);

    if ((m_platformInfo.eProductFamily == IGFX_DG2 && isChildOfDG2C0) ||
        (m_platformInfo.eProductFamily == IGFX_DG2 && isDG2G11Config && isChildOfDG2B0) ||
        (m_platformInfo.eProductFamily == IGFX_DG2 && isDG2G12Config ) ||
        (m_platformInfo.eRenderCoreFamily == IGFX_XE_HPC_CORE))
    {
        return IGC_IS_FLAG_DISABLED(ForceFormatConversionDG2Plus);
    }

    return false;
}

bool needsWAForThreadsUtilization() const
{
    return m_platformInfo.eProductFamily == IGFX_DG2;
}

bool supportDualSimd8PS() const
{
    return IGC_IS_FLAG_ENABLED(EnableDualSIMD8) && (m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE);
}

bool hasDualSimd8Payload() const
{
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE;
}

unsigned int getMaxMeshShaderThreads() const {
    return m_caps.MediaShaderThreads - 1;
}

bool supportDpaswInstruction() const
{
    return hasFusedEU() && supportDpasInstruction();
}

// This represents the max number of logical lanes available for RT,
// so it is not dependent on compiled SIMD size or "PreferredRayTracingSIMDSize".
// The calculation here is: (ThreadCount / DualSubSliceCount) * 16,
// where 16 is max current SIMD lenght for RT.
unsigned getRTStackDSSMultiplier() const
{
    IGC_ASSERT(supportRayTracing());
    return 2048;
}

// Max number of hw threads for each workgroup
unsigned int getMaxNumberHWThreadForEachWG() const
{
    if (m_platformInfo.eRenderCoreFamily < IGFX_GEN12_CORE) {
        // Each WG is dispatched into one subslice for GEN11 and before
        return getMaxNumberThreadPerSubslice();
    }
    else if (m_platformInfo.eRenderCoreFamily <= IGFX_XE_HPC_CORE)
    {
        // Each WG is dispatched into one DSS which has 2 Subslices for Gen12 and above
        if (getWATable().Wa_1609337546 || getWATable().Wa_1609337769) {
            return 64;
        }
        else {
            return getMaxNumberThreadPerSubslice() * 2;
        }
    }
    else {
        IGC_ASSERT_MESSAGE(0, "Unsupported platform!");
    }
    return 0;
}

uint32_t getGRFSize() const
{
    return isCoreChildOf(IGFX_XE_HPC_CORE) ? 64 : 32;
}

uint32_t getInlineDataSize() const
{
    return supportInlineData() ? 32 : 0;
}

bool hasFusedEU() const
{
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE &&
        !isCoreChildOf(IGFX_XE_HPC_CORE);
}

bool hasPartialEmuI64Enabled() const
{
    return hasPartialInt64Support() && IGC_IS_FLAG_ENABLED(EnablePartialEmuI64);
}


bool matchImmOffsetsLSC() const
{
    enum LscMatchImmMode {
        OFF = 0,
        ON = 2,
    };
    auto immOffsetMode = (LscMatchImmMode)IGC_GET_FLAG_VALUE(LscImmOffsMatch);
    return hasLSC() && immOffsetMode >= ON;
}

bool WaCubeHFPrecisionBug() const
{
    return m_WaTable.Wa_18012201914 != 0;
}

//The max size in bytes of the scratch space per thread.
//  XeHP_SDV and above are for each physical thread: 256k.
//  TGLLP and below are for each FFTID: 2M.
uint32_t maxPerThreadScratchSpace() const
{
    return (hasScratchSurface() ? 0x40000 : 0x200000);
}

bool supportAIParameterCombiningWithLODBiasEnabled() const
{
    return IGC_IS_FLAG_ENABLED(EnableAIParameterCombiningWithLODBias) &&
           (m_platformInfo.eProductFamily == IGFX_DG2 && SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_B0)) ||
           GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID) ||
           GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID);
}

bool useScratchSpaceForOCL() const
{
    // Disable using scratch surface for private memory on XeHP_SDV
    // because it does not support byte-aligned (byte-scattered) messages.
    if (hasScratchSurface()) {
        return LSCEnabled() &&
               IGC_IS_FLAG_ENABLED(EnableOCLScratchPrivateMemory) &&
               m_platformInfo.eRenderCoreFamily == IGFX_XE_HPC_CORE;
    }
    else {
        return IGC_IS_FLAG_ENABLED(EnableOCLScratchPrivateMemory);
    }
}


// Check if byte ALU operations are well supported. If not, promote byte to i16/i32.
bool supportByteALUOperation() const
{
    return !isCoreChildOf(IGFX_XE_HPC_CORE);
}

// Platform requires kernel arguments pulling.
bool HasKernelArguments() const
{
    return false;
}



bool has64BMediaBlockRW() const
{
    return IGC_IS_FLAG_ENABLED(Enable64BMediaBlockRW) && isProductChildOf(IGFX_XE_HP_SDV);
}

bool supportsStaticRegSharing() const
{
    return isProductChildOf(IGFX_XE_HP_SDV);
}
bool emulateByteScraterMsgForSS() const
{
    return isProductChildOf(IGFX_XE_HP_SDV) && (m_platformInfo.usRevId == 0 || IGC_IS_FLAG_ENABLED(EnableUntypedSurfRWofSS));
}

//all the platforms which DONOT support 64 bit int operations
bool hasNoInt64Inst() const {
    return m_platformInfo.eProductFamily == IGFX_ICELAKE_LP ||
        m_platformInfo.eProductFamily == IGFX_LAKEFIELD ||
        m_platformInfo.eProductFamily == IGFX_ELKHARTLAKE ||
        m_platformInfo.eProductFamily == IGFX_JASPERLAKE ||
        m_platformInfo.eProductFamily == IGFX_TIGERLAKE_LP ||
        m_platformInfo.eProductFamily == IGFX_ROCKETLAKE ||
        m_platformInfo.eProductFamily == IGFX_ALDERLAKE_S ||
        m_platformInfo.eProductFamily == IGFX_ALDERLAKE_P ||
        m_platformInfo.eProductFamily == IGFX_ALDERLAKE_N ||
        m_platformInfo.eProductFamily == IGFX_DG1 ||
        m_platformInfo.eProductFamily == IGFX_DG2;
}

//all the platforms which DONOT support 64 bit float operations
bool hasNoFP64Inst() const {
    return m_platformInfo.eProductFamily == IGFX_ICELAKE_LP ||
        m_platformInfo.eProductFamily == IGFX_LAKEFIELD ||
        m_platformInfo.eProductFamily == IGFX_ELKHARTLAKE ||
        m_platformInfo.eProductFamily == IGFX_JASPERLAKE ||
        m_platformInfo.eProductFamily == IGFX_TIGERLAKE_LP ||
        m_platformInfo.eProductFamily == IGFX_ROCKETLAKE ||
        m_platformInfo.eProductFamily == IGFX_ALDERLAKE_S ||
        m_platformInfo.eProductFamily == IGFX_ALDERLAKE_P ||
        m_platformInfo.eProductFamily == IGFX_ALDERLAKE_N ||
        m_platformInfo.eProductFamily == IGFX_DG1 ||
        m_platformInfo.eProductFamily == IGFX_DG2;
}

//all the platforms which have correctly rounded macros (INVM, RSQRTM, MADM)
bool hasCorrectlyRoundedMacros() const {
    return m_platformInfo.eProductFamily != IGFX_ICELAKE_LP &&
        m_platformInfo.eProductFamily != IGFX_LAKEFIELD &&
        m_platformInfo.eProductFamily != IGFX_JASPERLAKE &&
        m_platformInfo.eProductFamily != IGFX_TIGERLAKE_LP &&
        m_platformInfo.eProductFamily != IGFX_ROCKETLAKE &&
        m_platformInfo.eProductFamily != IGFX_DG1 &&
        m_platformInfo.eProductFamily != IGFX_ALDERLAKE_S &&
        m_platformInfo.eProductFamily != IGFX_ALDERLAKE_P &&
        m_platformInfo.eProductFamily != IGFX_ALDERLAKE_N &&
        m_platformInfo.eProductFamily != IGFX_DG2;
}

bool supportMixMode() const {
    return IGC_IS_FLAG_ENABLED(ForceMixMode) ||
        (IGC_IS_FLAG_DISABLED(DisableMixMode) &&
        (m_platformInfo.eProductFamily == IGFX_CHERRYVIEW ||
            m_platformInfo.eRenderCoreFamily == IGFX_GEN9_CORE ||
            m_platformInfo.eRenderCoreFamily == IGFX_GEN10_CORE));
}
bool DSPrimitiveIDPayloadPhaseCanBeSkipped() const { return false; }

bool NeedsHDCFenceBeforeEOTInPixelShader() const
{
    return m_WaTable.Wa_1807084924 != 0;
}

bool canFuseTypedWrite() const
{
    return false;
}

bool enableSetDefaultTileYWalk() const
{
    return false;
}

// max block size for legacy OWord block messages
uint32_t getMaxBlockMsgSize(bool isSLM) const
{
    return 128;
}

int getBSOLocInExtDescriptor() const
{
    return 12;
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

bool WaDisableStaticRegSharing() const
{
    return m_WaTable.Wa_14012688715 != 0;
}

bool WaDisablePrimitiveReplicationWithCPS() const
{
    return m_WaTable.Wa_18013852970 != 0;
}

bool supportSystemFence() const
{
    return hasLSC()
        && m_platformInfo.eProductFamily != IGFX_DG2;
}


bool WaGeoShaderURBAllocReduction() const
{
    return m_WaTable.Wa_18012660806 != 0;
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

bool WaDisableSampleLz() const
{
    return (IGC_IS_FLAG_DISABLED(DisableWaSampleLZ) && m_WaTable.Wa_14013297064);
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

// Return true if platform has structured control-flow instructions and IGC wants to use them.
bool hasSCF() const
{
    bool doscf = true;
    // DG2 and PVC still has SCF, but igc will stop using them.
    doscf = !isProductChildOf(IGFX_DG2);
    return doscf;
}

const SCompilerHwCaps& GetCaps() { return m_caps; }

bool supportHeaderRTW() const
{
    return true;
}

bool preemptionSupported() const
{
    if (isProductChildOf(IGFX_PVC))
        return false;

    return GetPlatformFamily() >= IGFX_GEN9_CORE;
}

// platform natively not support DW-DW multiply
bool noNativeDwordMulSupport() const
{
    return m_platformInfo.eProductFamily == IGFX_BROXTON ||
        m_platformInfo.eProductFamily == IGFX_GEMINILAKE ||
        m_platformInfo.eProductFamily == IGFX_DG2 ||
        GetPlatformFamily() == IGFX_GEN11_CORE ||
        GetPlatformFamily() == IGFX_GEN12LP_CORE;
}

unsigned getURBFullWriteMinGranularity() const
{
    unsigned overrideValue = IGC_GET_FLAG_VALUE(SetURBFullWriteGranularity);
    if (overrideValue == 16 || overrideValue == 32)
    {
        return overrideValue;
    }
    return isProductChildOf(IGFX_XE_HP_SDV) ? 16 : 32; // in bytes
}

unsigned forceQwAtSrc0ForQwShlWA() const
{
    // PVC XT A0: RevID==0X3==REVISION_B
    return (m_platformInfo.eProductFamily == IGFX_PVC && m_platformInfo.usRevId == REVISION_B);
}


bool hasSIMD8Support() const
{
    return !(m_platformInfo.eRenderCoreFamily == IGFX_XE_HPC_CORE);
}

bool hasThreadPauseSupport() const
{
    if (isCoreChildOf(IGFX_XE_HPC_CORE))
        return false;
    return true;
}
};
}//namespace IGC
