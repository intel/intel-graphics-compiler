/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

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

#include "skuwa/iacm_g10_rev_id.h"
#include "skuwa/iacm_g11_rev_id.h"
#include "skuwa/iacm_g12_rev_id.h"

#include "iStdLib/utility.h"
#include "visa_igc_common_header.h"

namespace IGC {

class CPlatform {
  PLATFORM m_platformInfo = {};
  SCompilerHwCaps m_caps = {};
  WA_TABLE m_WaTable = {};
  SKU_FEATURE_TABLE m_SkuTable = {};
  GT_SYSTEM_INFO m_GTSystemInfo = {};
  OCLCaps m_OCLCaps = {};

public:
  CPlatform() {}

  CPlatform(const PLATFORM &platform) { m_platformInfo = platform; }

private:
  bool hasQWAddSupport() const {
    return (m_platformInfo.eProductFamily == IGFX_PVC &&
            m_platformInfo.usRevId >= REVISION_D) // true from PVC XT B0 RevID==0x5==REVISION_D
           || isCoreChildOf(IGFX_XE2_HPG_CORE);
  }

  // all the platforms which DONOT support 64 bit int operations
  bool hasNoInt64Inst() const {
    return m_platformInfo.eProductFamily == IGFX_ICELAKE_LP ||
           m_platformInfo.eProductFamily == IGFX_LAKEFIELD ||
           // m_platformInfo.eProductFamily == IGFX_ELKHARTLAKE || // same enum as JASPERLAKE
           m_platformInfo.eProductFamily == IGFX_JASPERLAKE || m_platformInfo.eProductFamily == IGFX_TIGERLAKE_LP ||
           m_platformInfo.eProductFamily == IGFX_ROCKETLAKE || m_platformInfo.eProductFamily == IGFX_ALDERLAKE_S ||
           m_platformInfo.eProductFamily == IGFX_ALDERLAKE_P || m_platformInfo.eProductFamily == IGFX_ALDERLAKE_N ||
           m_platformInfo.eProductFamily == IGFX_DG1 || m_platformInfo.eProductFamily == IGFX_DG2 ||
           m_platformInfo.eProductFamily == IGFX_METEORLAKE || m_platformInfo.eProductFamily == IGFX_ARROWLAKE;
  }

public:
  void setOclCaps(OCLCaps &caps) { m_OCLCaps = caps; }
  uint32_t getMaxOCLParameteSize() const {
    uint32_t limitFromFlag = IGC_GET_FLAG_VALUE(OverrideOCLMaxParamSize);
    return limitFromFlag ? limitFromFlag : m_OCLCaps.MaxParameterSize;
  }
  void OverrideRevId(unsigned short newRevId) { m_platformInfo.usRevId = newRevId; }

  void OverrideDeviceId(unsigned short newDeviceID) { m_platformInfo.usDeviceID = newDeviceID; }

  void OverrideProductFamily(unsigned int productID) {
    PRODUCT_FAMILY eProd = static_cast<PRODUCT_FAMILY>(productID);
    if (eProd > IGFX_UNKNOWN && eProd < IGFX_MAX_PRODUCT)
      m_platformInfo.eProductFamily = (PRODUCT_FAMILY)productID;
  }

  WA_TABLE const &getWATable() const { return m_WaTable; }
  SKU_FEATURE_TABLE const &getSkuTable() const { return m_SkuTable; }

  bool hasPackedVertexAttr() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
  bool hasScaledMessage() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
  bool has8ByteA64ByteScatteredMessage() const { return m_platformInfo.eRenderCoreFamily == IGFX_GEN8_CORE; }
  bool hasQWGatherScatterBTSMessage() const { return m_platformInfo.eRenderCoreFamily >= IGFX_XE_HP_SDV; }

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
  bool supportsCoordinateNormalizationForRectangleTextures() const {
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE;
  }

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
  bool supportsHDCLegacyDCROMessage() const { return m_platformInfo.eRenderCoreFamily <= IGFX_XE_HPC_CORE; }

  bool hasEfficient64bEnabled() const {
    return m_SkuTable.FtrEfficient64BitAddressing ||
           (isCoreChildOf(IGFX_XE3P_CORE) && IGC_IS_FLAG_ENABLED(EnableEfficient64b));
  }

  bool hasSWManagedStackCountEnabled() const {
    return isCoreChildOf(IGFX_XE3P_CORE) && IGC_IS_FLAG_DISABLED(DisableSWManagedStack);
  }


  bool hasEngineIDEnabled() const { return isCoreChildOf(IGFX_XE3P_CORE) && IGC_IS_FLAG_DISABLED(DisableEngineID); }
  bool SupportSurfaceInfoMessage() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
  bool SupportHDCUnormFormats() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE; }
  bool localMemFenceSupress() const {
    return m_platformInfo.eRenderCoreFamily <= IGFX_GEN9_CORE || IGC_IS_FLAG_ENABLED(DisbleLocalFences);
  }
  bool psSimd32SkipStallHeuristic() const { return m_caps.KernelHwCaps.EUThreadsPerEU == 6; }
  bool enablePSsimd32() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }

  bool supportSimd32PerPixelPSWithNumSamples16() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }

  bool canSupportWMTPWithoutBTD() const {
    // Returns true if the platform has the capability of supporting
    // WMTP but WMTP isn't supported for all shader types
    return (isCoreChildOf(IGFX_XE2_HPG_CORE) && !isCoreChildOf(IGFX_XE3_CORE));
  }


  bool canSupportWMTP() const {
    // Returns true if the platform has the capability of supporting
    // WMTP but WMTP isn't supported for all shader types
    return isCoreChildOf(IGFX_XE2_HPG_CORE);
  }

  bool supportsWMTPForShaderType(ShaderType type) const {
    if (isCoreChildOf(IGFX_XE3_CORE)) {
      switch (type) {
      case ShaderType::COMPUTE_SHADER:
      case ShaderType::OPENCL_SHADER:
      case ShaderType::RAYTRACING_SHADER:
        return true;
      default:
        return false;
      }
    }

    if (isCoreChildOf(IGFX_XE2_HPG_CORE)) {
      switch (type) {
      case ShaderType::COMPUTE_SHADER:
      case ShaderType::OPENCL_SHADER:
        return true;
      default:
        return false;
      }
    }

    return false;
  }

  bool supportDisableMidThreadPreemptionSwitch() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE; }

  bool needSWStencil() const {
    return (m_platformInfo.eRenderCoreFamily == IGFX_GEN9_CORE && IGC_IS_FLAG_ENABLED(EnableSoftwareStencil));
  }
  bool supportMSAARateInPayload() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE; }

  bool support16BitImmSrcForMad() const { return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE); }

  // This function checks if product is child of another product
  bool isProductChildOf(PRODUCT_FAMILY product) const {
    if (product == IGFX_PVC)
      return isCoreChildOf(IGFX_XE_HPC_CORE);
    return m_platformInfo.eProductFamily >= product;
  }

  bool isPVC() const { return (m_platformInfo.eProductFamily == IGFX_PVC); }
  bool isCoreXE2() const { return (m_platformInfo.eRenderCoreFamily == IGFX_XE2_HPG_CORE); }

  bool isCoreXE3() const {
    return (m_platformInfo.eRenderCoreFamily == IGFX_XE3_CORE || m_platformInfo.eRenderCoreFamily >= IGFX_XE3P_CORE);
  }

  // This function checks if core is child of another core
  bool isCoreChildOf(GFXCORE_FAMILY core) const { return m_platformInfo.eRenderCoreFamily >= core; }

  bool supports8DWLSCMessage() const {
    return (SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_B0) && m_platformInfo.eProductFamily == IGFX_DG2) ||
           GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID) || GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID)
           || isCoreChildOf(IGFX_XE_HPC_CORE);
  }

  PRODUCT_FAMILY GetProductFamily() const { return m_platformInfo.eProductFamily; }
  unsigned short GetDeviceId() const { return m_platformInfo.usDeviceID; }
  unsigned short GetRevId() const { return m_platformInfo.usRevId; }
  GFXCORE_FAMILY GetPlatformFamily() const { return m_platformInfo.eRenderCoreFamily; }
  const PLATFORM &getPlatformInfo() const { return m_platformInfo; }
  void SetCaps(const SCompilerHwCaps &caps) { m_caps = caps; }
  void SetWATable(const WA_TABLE &waTable) { m_WaTable = waTable; }
  void SetSkuTable(const SKU_FEATURE_TABLE &skuTable) { m_SkuTable = skuTable; }
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

  void SetGTSystemInfo(const GT_SYSTEM_INFO &gtSystemInfo) { m_GTSystemInfo = gtSystemInfo; }

  GT_SYSTEM_INFO GetGTSystemInfo() const { return m_GTSystemInfo; }

  unsigned int getMaxPixelShaderThreads() const {
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE ? m_caps.PixelShaderThreadsWindowerRange - 1
                                                              : m_caps.PixelShaderThreadsWindowerRange - 2;
  }
  bool supportGPGPUMidThreadPreemption() const {
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE && m_platformInfo.eRenderCoreFamily <= IGFX_GEN11LP_CORE;
  }
  bool supportFtrWddm2Svm() const { return m_SkuTable.FtrWddm2Svm != 0; }
  bool supportStructuredAsRaw() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE; }
  bool supportSamplerCacheResinfo() const { return m_platformInfo.eRenderCoreFamily == IGFX_GEN8_CORE; }

  unsigned int getMaxVertexShaderThreads(const bool isPositionOnlyShader) const {
    const unsigned int maxVertexShaderThreads =
        isPositionOnlyShader ? m_caps.VertexShaderThreadsPosh : m_caps.VertexShaderThreads;
    return maxVertexShaderThreads - 1;
  }
  unsigned int getMaxGeometryShaderThreads() const { return m_caps.GeometryShaderThreads - 1; }
  unsigned int getMaxHullShaderThreads() const { return m_caps.HullShaderThreads - 1; }
  unsigned int getMaxDomainShaderThreads() const { return m_caps.DomainShaderThreads - 1; }
  unsigned int getMaxGPGPUShaderThreads() const { return m_caps.MediaShaderThreads - 1; }
  unsigned int getKernelPointerAlignSize() const { return m_caps.KernelHwCaps.KernelPointerAlignSize; }
  unsigned int getSharedLocalMemoryBlockSize() const { return m_caps.SharedLocalMemoryBlockSize; }
  unsigned int getMaxNumberThreadPerSubslice() const {
    // total number of threads per subslice
    if (m_caps.KernelHwCaps.SubSliceCount != 0)
      return m_caps.KernelHwCaps.ThreadCount / m_caps.KernelHwCaps.SubSliceCount;
    return 0;
  }
  unsigned int getMaxNumberThreadPerWorkgroupPooledMax() const {
    return m_caps.KernelHwCaps.EUCountPerPoolMax * m_caps.KernelHwCaps.EUThreadsPerEU;
  }
  unsigned int getMaxSimdSize() const { return 32; }
  unsigned int getBarrierCountBits(unsigned int count) const {
    // Returns barrier count field + enable for barrier message
    if (m_platformInfo.eRenderCoreFamily <= IGFX_GEN10_CORE) {
      // up to Gen9 barrier count is in bits 14:9, enable is bit 15
      return (count << 9) | (1 << 15);
    } else {
      // for Gen10+ barrier count is in bits 14:8, enable is bit 15
      return (count << 8) | (1 << 15);
    }
  }

  bool supportsDrawParametersSGVs() const {
    // Gen10+, 3DSTATE_VF_SGVS_2
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE;
  }

  bool hasPSDBottleneck() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE; }

  bool supportsHardwareResourceStreamer() const { return m_platformInfo.eRenderCoreFamily < IGFX_GEN11_CORE; }
  bool AOComputeShadersSIMD32Mode() const { return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE); }
  unsigned int getHullShaderThreadInstanceIdBitFieldPosition() const {
    // HS thread receives instance ID in R0.2 bits 22:16 for Gen10+ and bits 23:17 for older Gens
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE) ? 16 : 17;
  }
  bool supportsBinaryAtomicCounterMessage() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE; }
  bool supportSLMBlockMessage() const { return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE); }

  bool hasSLMFence() const { return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE); }

  bool hasIndependentSharedMemoryFenceFunctionality() const { return (m_platformInfo.eProductFamily != IGFX_DG2); }

  bool supportRotateInstruction() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE; }
  bool supportLRPInstruction() const { return m_platformInfo.eRenderCoreFamily < IGFX_GEN11_CORE; }
  bool support16bitMSAAPayload() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE; }
  bool supportTwoStackTSG() const {
    // Will need check for specific skus where TwoStackTSG is enabled
    // Not all skus have it enabled
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE);
  }
  bool enableBlendToDiscardAndFill() const { return (m_platformInfo.eRenderCoreFamily < IGFX_GEN11_CORE); }
  bool HSUsesHWBarriers() const {
    // HS HW barriers work correctly since ICL platform.
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE);
  }

  bool NeedResetA0forVxHA0() const {
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE && !isCoreChildOf(IGFX_XE3_CORE));
  }


  unsigned int GetBindlessSamplerSize() const {
    if (isCoreChildOf(IGFX_XE3P_CORE) && IGC_IS_FLAG_ENABLED(Enable32bSampler)) {
      // Samplers are 32 bytes
      return 32;
    } else {
      // Samplers are 16 bytes
      return 16;
    }
  }

  unsigned int GetLogBindlessSamplerSize() const { return (unsigned int)iSTD::Log2(GetBindlessSamplerSize()); }

  bool SupportCPS() const { return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN10_CORE); }
  bool hasUnifiedCoarseAndPixelDispatchRates() const { return (m_platformInfo.eRenderCoreFamily >= IGFX_XE3_CORE); }
  bool supportsSIMD32forCPS() const { return (m_platformInfo.eProductFamily >= IGFX_METEORLAKE); }

  bool supportsThreadCombining() const {
    return (!(!m_WaTable.WaEnablePooledEuFor2x6 && m_platformInfo.eProductFamily == IGFX_BROXTON &&
              m_GTSystemInfo.SubSliceCount == 2)) &&
           (m_platformInfo.eRenderCoreFamily < IGFX_GEN12_CORE);
  }

  bool enableMaxWorkGroupSizeCalculation() const {
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE) && IGC_IS_FLAG_ENABLED(EnableMaxWGSizeCalculation);
  }

  bool has8DWA64ScatteredMessage() const { return m_platformInfo.eRenderCoreFamily < IGFX_GEN12_CORE; }

  bool flushL3ForTypedMemory() const { return m_platformInfo.eRenderCoreFamily <= IGFX_GEN11_CORE; }

  bool supportsStencil(SIMDMode simdMode) const {
    return getMinDispatchMode() == SIMDMode::SIMD16 ? true : simdMode == SIMDMode::SIMD8;
  }

  bool hasFDIV() const {
    if (IGC_IS_FLAG_ENABLED(DisableFDIV))
      return false;
    return (m_platformInfo.eRenderCoreFamily < IGFX_GEN12_CORE);
  }

  bool doIntegerMad(bool bothOperandsAreUniform = false) const {
    if (bothOperandsAreUniform && m_platformInfo.eProductFamily == IGFX_METEORLAKE)
      return false;

    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN11_CORE && m_platformInfo.eProductFamily != IGFX_DG1 &&
           IGC_IS_FLAG_ENABLED(EnableIntegerMad);
  }

  bool supportsSourceModifierForMixedIntMad() const {
    if (isCoreChildOf(IGFX_XE3P_CORE))
      return true;
    return false;
  }

  bool isDG1() const { return m_platformInfo.eProductFamily == IGFX_DG1; }

  bool simplePushIsFasterThanGather() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE; }

  bool singleThreadBasedInstScheduling() const { return m_platformInfo.eRenderCoreFamily < IGFX_GEN12_CORE; }

  // all the platforms which do not support 64 bit operations and
  // needs int64 emulation support. Except also for BXT where
  // 64-bit inst has much lower throughput compared to SKL.
  // Emulating it improves performance on some benchmarks and
  // won't have impact on the overall performance.
  bool need64BitEmulation() const {
    return m_platformInfo.eProductFamily == IGFX_GEMINILAKE || m_platformInfo.eProductFamily == IGFX_BROXTON ||
           hasNoInt64Inst();
  }

  bool HDCCoalesceSLMAtomicINCWithNoReturn() const { return m_platformInfo.eRenderCoreFamily == IGFX_GEN12_CORE; }

  int LSCCachelineSize() const { return 64; }


  bool HDCCoalesceAtomicCounterAccess() const {
    return (m_platformInfo.eRenderCoreFamily < IGFX_GEN12_CORE) &&
           IGC_IS_FLAG_DISABLED(ForceSWCoalescingOfAtomicCounter);
  }

  bool supportsMCSNonCompressedFix() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE; }

  bool hasHWDp4AddSupport() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE; }

  bool useOnlyEightPatchDispatchHS() const { return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE); }

  bool supportsPrimitiveReplication() const {
    return ((m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE) ||
            (m_platformInfo.eRenderCoreFamily == IGFX_GEN11_CORE && m_platformInfo.eProductFamily == IGFX_ICELAKE));
  }

  // If true then screen space coordinates for upper-left vertex of a triangle
  // being rasterized are delivered together with source depth or W deltas.
  bool hasStartCoordinatesDeliveredWithDeltas() const { return m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE; }

  bool disableStaticVertexCount() const {
    return m_WaTable.Wa_14012504847 != 0 || IGC_IS_FLAG_ENABLED(ForceStaticToDynamic);
  }

  bool hasSamplerSupport() const {
    return (m_platformInfo.eRenderCoreFamily != IGFX_XE_HPC_CORE) ||
           (IGC_IS_FLAG_ENABLED(EnableSamplerSupport)); // flag for IGFX_PVC
  }

  bool hasSamplerFeedbackSurface() const { return m_platformInfo.eProductFamily >= IGFX_BMG; }

  // logical subslice id
  bool hasLogicalSSID() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }

  uint32_t getMinPushConstantBufferAlignment() const {
    return 8; // DWORDs
  }

  // Returns the default limit of pushed constant data in 32-byte units. This value limits
  // the amount of constant buffer data promoted to registers.
  uint32_t getBlockPushConstantThreshold() const {
    constexpr uint32_t defaultThreshold = 31;
    constexpr uint32_t gen9GT2Threshold = 15;

    const GTTYPE gt = m_platformInfo.eGTType;
    switch (m_platformInfo.eProductFamily) {
    case IGFX_COFFEELAKE:
    case IGFX_SKYLAKE:
    case IGFX_KABYLAKE:
      return (gt == GTTYPE_GT2) ? gen9GT2Threshold : defaultThreshold;
    default:
      return defaultThreshold;
    }
  }

  bool hasDualSubSlices() const {
    bool hasDualSS =
        m_platformInfo.eRenderCoreFamily == IGFX_GEN12_CORE || m_platformInfo.eRenderCoreFamily == IGFX_GEN12LP_CORE;
    if (m_platformInfo.eRenderCoreFamily == IGFX_XE_HPG_CORE) {
      hasDualSS = true;
    }
    return hasDualSS;
  }

  unsigned getSlmSizePerSsOrDss() const {
    // GTSysInfo sets SLMSize only for gen12+
    unsigned slmSizePerSsOrDss = 65536;
    if (hasDualSubSlices()) {
      if (GetGTSystemInfo().DualSubSliceCount) {
        slmSizePerSsOrDss = GetGTSystemInfo().SLMSizeInKb / GetGTSystemInfo().DualSubSliceCount * 1024;
      } else {
        slmSizePerSsOrDss = 131072;
      }
    } else if (isProductChildOf(IGFX_BMG)) {
      // SLMSizeInKb is reported per SS for BMG+
      slmSizePerSsOrDss = GetGTSystemInfo().SLMSizeInKb * 1024;
    } else if (isProductChildOf(IGFX_PVC)) {
      slmSizePerSsOrDss = GetGTSystemInfo().SLMSizeInKb / GetGTSystemInfo().SubSliceCount * 1024;
    }
    return slmSizePerSsOrDss;
  }

  bool canForcePrivateToGlobal() const {
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN9_CORE &&
           IGC_IS_FLAG_ENABLED(ForcePrivateMemoryToGlobalOnGeneric);
  }

  bool getHWTIDFromSR0() const { return isProductChildOf(IGFX_XE_HP_SDV); }

  bool supportAdd3Instruction() const { return isProductChildOf(IGFX_XE_HP_SDV); }

  bool supportBfnInstruction() const { return isProductChildOf(IGFX_XE_HP_SDV); }

  bool supportDpasInstruction() const {
    return isProductChildOf(IGFX_XE_HP_SDV) && m_platformInfo.eProductFamily != IGFX_METEORLAKE &&
           !GFX_IS_ARL_S(m_platformInfo.usDeviceID) &&
           !(m_platformInfo.eProductFamily == IGFX_PVC && GFX_IS_VG_CONFIG(m_platformInfo.usDeviceID));
  }

  bool supportJointMatrixOCLExtension() const {
    return supportDpasInstruction() ||
           // SPV_INTEL_joint_matrix extension is partially supported on PVC-VG. Only OpJointMatrixMadINTEL
           // opcodes are not supported and proper error is emitted in case any of them is used in a kernel.
           (m_platformInfo.eProductFamily == IGFX_PVC && GFX_IS_VG_CONFIG(m_platformInfo.usDeviceID));
  }

  bool hasPackedRestrictedFloatVector() const {
    return !isCoreChildOf(IGFX_XE2_HPG_CORE);
  }

  bool supportLoadThreadPayloadForCompute() const { return isProductChildOf(IGFX_XE_HP_SDV); }

  bool support16BitAtomics() const { return m_platformInfo.eProductFamily >= IGFX_GEN12_CORE; }

  bool Enable32BitIntDivRemEmu() const { return isProductChildOf(IGFX_XE_HP_SDV); }

  bool support16DWURBWrite() const {
    return IGC_IS_FLAG_ENABLED(Enable16DWURBWrite) && isProductChildOf(IGFX_XE_HP_SDV);
  }

  bool hasScratchSurface() const { return isProductChildOf(IGFX_XE_HP_SDV); }

  bool has16MBPerThreadScratchSpace() const {
    return isProductChildOf(IGFX_CRI);
  }
  bool canCoalesceAtomicWithNoReturn() const {
    return isProductChildOf(IGFX_CRI);
  }

  bool hasAtomicPreDec() const { return !isProductChildOf(IGFX_XE_HP_SDV); }

  bool support26BitBSOFormat() const { return isProductChildOf(IGFX_XE_HP_SDV); }

  bool needsHeaderForAtomicCounter() const { return isProductChildOf(IGFX_XE_HP_SDV); }

  bool doScalar64bScan() const { return isProductChildOf(IGFX_XE_HP_SDV); }

  bool hasHWLocalThreadID() const { return isProductChildOf(IGFX_XE_HP_SDV); }

  unsigned int getOfflineCompilerMaxWorkGroupSize() const {
    if (isProductChildOf(IGFX_XE_HP_SDV))
      return 1024;
    return 448;
  }

  bool hasFP16AtomicMinMax() const { return isProductChildOf(IGFX_XE_HP_SDV); }

  bool hasFP32GlobalAtomicAdd() const { return isProductChildOf(IGFX_XE_HP_SDV); }

  bool hasFP32LocalAtomicAdd() const {
    return isCoreChildOf(IGFX_XE3P_CORE) && IGC_IS_FLAG_ENABLED(EnableNativeFP32LocalAtomicAdd);
  }

  bool has16OWSLMBlockRW() const {
    return IGC_IS_FLAG_ENABLED(Enable16OWSLMBlockRW) && isProductChildOf(IGFX_XE_HP_SDV);
  }

  bool hasLargeMaxConstantBufferSize() const {
    return IGC_IS_FLAG_DISABLED(Force32bitConstantGEPLowering) && isCoreChildOf(IGFX_XE_HPC_CORE);
  }

  bool supportInlineData() const { return isProductChildOf(IGFX_XE_HP_SDV); }

  // TODO: temporary solution, remove this once it's not needed
  bool supportInlineDataOCL() const {
    if (m_platformInfo.eRenderCoreFamily == IGFX_XE_HPC_CORE && IGC_IS_FLAG_DISABLED(ForceInlineDataForXeHPC)) {
      return false;
    }
    return isProductChildOf(IGFX_XE_HP_SDV);
  }

  bool supportsAutoGRFSelection() const {
    return isProductChildOf(IGFX_PVC) || m_platformInfo.eProductFamily == IGFX_XE_HP_SDV ||
           (m_platformInfo.eProductFamily == IGFX_DG2 && IGC_IS_FLAG_ENABLED(ForceSupportsAutoGRFSelection));
  }

  bool hasLSC() const {
    return IGC_IS_FLAG_DISABLED(ForceNoLSC) && !WaEnableLSCBackupMode() && isProductChildOf(IGFX_DG2);
  }

  bool WaEnableLSCBackupMode() const { return (m_WaTable.Wa_14010198302 != 0); }

  bool supportQWRotateInstructions() const {
    return m_platformInfo.eRenderCoreFamily == IGFX_XE_HPC_CORE && IGC_IS_FLAG_ENABLED(EnableQWRotateInstructions);
  }

  bool isIntegratedGraphics() const {
    switch (m_platformInfo.eProductFamily) {
    case IGFX_DG1:
    case IGFX_XE_HP_SDV:
    case IGFX_DG2:
    case IGFX_PVC:
    case IGFX_BMG:
    case IGFX_CRI:
      return false;
    default:
      return true;
    }
  }

  bool loosenSimd32occu() const {
    if (IGC_GET_FLAG_VALUE(ForceLoosenSimd32Occu) == 2)
      return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE && m_platformInfo.eProductFamily != IGFX_DG2);
    else
      return IGC_GET_FLAG_VALUE(ForceLoosenSimd32Occu);
  }

  // Local memory here refers to memory on the device- e.g. HBM for PVC.
  bool hasLocalMemory() const { return m_SkuTable.FtrLocalMemory != 0; }

  bool supportsTier2VRS() const { return m_platformInfo.eProductFamily >= IGFX_DG2; }

  bool supportsSIMD16TypedRW() const { return isCoreChildOf(IGFX_XE_HPC_CORE); }

  bool supportsBGRATypedRead() const { return isCoreChildOf(IGFX_XE_HP_CORE); }

  bool supportHWGenerateTID() const {
    return IGC_IS_FLAG_ENABLED(EnableHWGenerateThreadID) && isProductChildOf(IGFX_DG2);
  }

  bool supportHWDispatchWalkOrder() const {
    const bool hasHWDispatchWalkOrder =
        (m_platformInfo.eProductFamily == IGFX_ARROWLAKE && !(GFX_IS_ARL_S(m_platformInfo.usDeviceID)))
        || isCoreChildOf(IGFX_XE2_HPG_CORE);
    return hasHWDispatchWalkOrder;
  }

  bool hasHalfSIMDLSC() const {
    return (m_platformInfo.eProductFamily == IGFX_DG2 && SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_B0)) ||
           GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID) || GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID) ||
           // false for PVC XL A0 RevID==0x0, true from PVC XT A0 RevID==0x3==REVISION_B
           (m_platformInfo.eProductFamily == IGFX_PVC && m_platformInfo.usRevId >= REVISION_B) ||
           m_platformInfo.eProductFamily == IGFX_METEORLAKE || m_platformInfo.eProductFamily == IGFX_ARROWLAKE ||
           isCoreChildOf(IGFX_XE2_HPG_CORE);
  }

  bool NeedsLSCFenceUGMBeforeEOT() const { return getWATable().Wa_22013689345 != 0; }

  bool hasPartialInt64Support() const {
    // false for PVC XL A0 RevID==0x0, true from PVC XT A0 RevID==0x3==REVISION_B
    return (m_platformInfo.eProductFamily == IGFX_PVC && m_platformInfo.usRevId >= REVISION_B) ||
           isCoreChildOf(IGFX_XE2_HPG_CORE);
  }

  bool hasInt64Add() const { return !hasNoFullI64Support() || hasQWAddSupport(); }

  bool hasFullInt64() const {
    // We don't support int64 adder in PVC-XT-A0, please see more info in hasQWAddSupport
    if (m_platformInfo.eProductFamily == IGFX_PVC && m_platformInfo.usRevId < REVISION_D) {
      return false;
    } else {
      return !hasNoInt64Inst();
    }
  }

  bool hasInt64DstMul() const {

    return (m_platformInfo.eProductFamily == IGFX_PVC && m_platformInfo.usRevId < REVISION_B) ||
           isCoreChildOf(IGFX_XE2_HPG_CORE);
  }

  bool hasFP64DPAS() const { return isCoreChildOf(IGFX_XE3P_CORE) && IGC_IS_FLAG_ENABLED(EnableFP64Dpas); }

  bool hasFP4DPAS() const { return isCoreChildOf(IGFX_XE3P_CORE) && IGC_IS_FLAG_ENABLED(EnableFP4Dpas); }

  bool hasExecSize16DPAS() const { return isCoreChildOf(IGFX_XE_HPC_CORE); }

  bool LSCSimd1NeedFullPayload() const {
    // in PVC XL A0 RevID=0x0, SIMD1 reads/writes need full payloads
    // this causes chaos for vISA (would need 4REG alignment)
    // and to make extra moves to enable the payload
    // PVC XT A0 RevID==0x3==REVISION_B gets this feature
    return (m_platformInfo.eProductFamily == IGFX_PVC && m_platformInfo.usRevId < REVISION_B);
  }

  bool hasNoFullI64Support() const { return (hasNoInt64Inst() || hasPartialInt64Support()); }

  bool hasBFTFDenormMode() const {
    return isCoreChildOf(IGFX_XE2_HPG_CORE);
  }
  bool hasMullh() const { return isCoreChildOf(IGFX_XE3P_CORE) && IGC_IS_FLAG_ENABLED(EnableMullh); }

  bool WaPredicatedStackIDRelease() const {
    return m_WaTable.Wa_22014559856 && IGC_IS_FLAG_DISABLED(DisablePredicatedStackIDRelease);
  }

  // This function was originally intended to return the MAXIMUM SIMD size supported by hardware
  // for shaders with ray queries. It is used by CompileSIMDSizeInCommon as an API-agnostic filter
  // to determine platform-specific constraints on which SIMD sizes can be compiled.
  //
  // However, at some point, shader type was added and the function began returning PREFERRED
  // SIMD sizes instead of maximum supported sizes. This caused CompileSIMDSizeInCommon to reject
  // non-preferred SIMD sizes, preventing users from forcing specific SIMD widths (e.g., via
  // ForceOCLSIMDWidth flag or reqd_sub_group_size attribute in OpenCL).
  //
  // For OpenCL (OPENCL_SHADER): This function now returns the maximum SIMD size supported by
  // the platform, NOT the preferred size. When OCL needs the preferred size, it calls
  // getPreferredRayQuerySIMDSize() instead.
  //
  // For other shader types (COMPUTE_SHADER, RAYTRACING_SHADER, PIXEL_SHADER): The preferred SIMD logic
  // is still embedded here for backward compatibility. These APIs should eventually migrate to
  // the same approach as OCL - using this function for max supported size and
  // getPreferredRayQuerySIMDSize() for preferred size.
  SIMDMode getMaxRayQuerySIMDSize(ShaderType shaderType) const {
      if (isCoreChildOf(IGFX_XE_HPG_CORE)) {
        return SIMDMode::SIMD16;
      } else {
        IGC_ASSERT_MESSAGE(0, "Unsupported platform!");
        return SIMDMode::UNKNOWN;
      }
  }

  // This returns the current maximum size that we recommend for performance.
  SIMDMode getPreferredRayQuerySIMDSize(ShaderType shaderType) const {
    SIMDMode ret = isCoreChildOf(IGFX_XE_HPC_CORE) ? SIMDMode::SIMD16 : SIMDMode::SIMD8;

    IGC_ASSERT_MESSAGE(ret <= getMaxRayQuerySIMDSize(shaderType),
                       "Preferred SIMD size for RayQuery must not be greater than MaxRayQuerySIMDSize!");

    return ret;
  }

  SIMDMode getPreferredRayTracingSIMDSize() const {
    return isCoreChildOf(IGFX_XE_HPC_CORE) ? SIMDMode::SIMD16 : SIMDMode::SIMD8;
  }

  bool supportRayTracing() const { return isProductChildOf(IGFX_DG2); }
  bool supportRayTracingSIMD32() const { return isCoreChildOf(IGFX_XE3P_CORE); }

  bool isValidNumThreads(int32_t numThreadsPerEU) const {
    return numThreadsPerEU == 0 // "auto" mode - use compiler heuristic
           || numThreadsPerEU == 4 || numThreadsPerEU == 10 || numThreadsPerEU == 8;
  }

  bool supports3DAndCubeSampleD() const {
    // Make sure to match hasSupportForSampleDOnCubeTextures LLVM3DBuilder\BuiltinsFrontend.hpp
    return (m_platformInfo.eProductFamily != IGFX_XE_HP_SDV && m_platformInfo.eProductFamily != IGFX_DG2 &&
            m_platformInfo.eProductFamily != IGFX_METEORLAKE && m_platformInfo.eProductFamily != IGFX_ARROWLAKE &&
            m_platformInfo.eProductFamily != IGFX_BMG && m_platformInfo.eProductFamily != IGFX_LUNARLAKE &&
            m_platformInfo.eRenderCoreFamily != IGFX_XE3_CORE && m_platformInfo.eRenderCoreFamily != IGFX_XE3P_CORE
    );
  }

  bool LSCEnabled(SIMDMode m = SIMDMode::UNKNOWN) const {
    if (IGC_IS_FLAG_ENABLED(EnableLSC))
      return true;
    if (hasLSC()) {
      switch (m_platformInfo.eProductFamily) {
      case IGFX_PVC:
        if (m == SIMDMode::UNKNOWN) {
          // Must generate LSC from PVC XT A0 RevID==0x3==REVISION_B (not include XL A0)
          return m_platformInfo.usRevId >= REVISION_B;
        }
        return (m == SIMDMode::SIMD16 && m_platformInfo.usRevId > REVISION_A0) || m == SIMDMode::SIMD32;
      case IGFX_DG2:
        if (m == SIMDMode::UNKNOWN) {
          // Must generate LSC after A0 (not include A0)
          return (SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_B0) ||
                  GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID) || GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID)
          );
        }
        return ((SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_B0) ||
                 GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID) || GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID)
                     ) &&
                m == SIMDMode::SIMD8) ||
               m == SIMDMode::SIMD16;
      case IGFX_METEORLAKE:
      case IGFX_ARROWLAKE:
        return m == SIMDMode::UNKNOWN || m == SIMDMode::SIMD8 || m == SIMDMode::SIMD16;
      default:
        return true;
      }
    }
    return false;
  }

  // Max LSC message block size is 8GRF
  uint32_t getMaxLSCBlockMsgSize(bool isD64 = true) const { return (isD64 ? 8 : 4) * getGRFSize(); }

  bool hasURBFence() const {
    return (m_platformInfo.eProductFamily == IGFX_DG2 || m_platformInfo.eProductFamily == IGFX_METEORLAKE ||
            m_platformInfo.eProductFamily == IGFX_ARROWLAKE || isCoreChildOf(IGFX_XE2_HPG_CORE));
  }

  bool hasMultiTile() const {
    // FIXME: what to do for AOT compile?
    return m_GTSystemInfo.MultiTileArchInfo.TileCount > 1;
  }

  // UGM LSC fence with GPU scope triggers L3 flush
  bool hasL3FlushOnGPUScopeInvalidate() const {
    return m_platformInfo.eProductFamily == IGFX_ARROWLAKE || m_platformInfo.eProductFamily == IGFX_METEORLAKE ||
           m_platformInfo.eProductFamily == IGFX_DG2;
  }

  bool L3CacheCoherentCrossTiles() const { return isCoreChildOf(IGFX_XE_HPC_CORE); }

  // if RayTracing fence WA for LSCBackupMode (DG2.A0 only actually) is enabled.
  // In case of RayQuery of DG2.A0, we have to flush L1 after RTWrite and before shader read.
  // The reason is, for RT writes, write - completion must guarantee that
  // subsequent read with LSC bypass can read.This issue can be present
  // even in async mode too but in sync mode read happens rather quickly
  // compared to BTD dispatched thread.
  bool RTFenceWAforBkModeEnabled() const { return WaEnableLSCBackupMode(); }

  SIMDMode getMinDispatchMode() const { return isCoreChildOf(IGFX_XE_HPC_CORE) ? SIMDMode::SIMD16 : SIMDMode::SIMD8; }

  bool hasLSCTypedMessage() const { return isCoreChildOf(IGFX_XE_HPC_CORE); }

  bool LSC2DSupportImmXY() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }

  SIMDMode getMaxLSCTypedMessageSize() const {
    switch (m_platformInfo.eRenderCoreFamily) {
    case IGFX_XE_HPG_CORE:
      return SIMDMode::SIMD8;
    case IGFX_XE_HPC_CORE:
      return SIMDMode::SIMD16;
    default:
      if (IGC_IS_FLAG_ENABLED(DisableLSCSIMD32TGMMessages)) {
        return SIMDMode::SIMD16;
      } else {
        return SIMDMode::SIMD32;
      }
    }
  }

  unsigned getAccChNumUD() const { return isCoreChildOf(IGFX_XE_HPC_CORE) ? 16 : 8; }

  bool hasInt64SLMAtomicCAS() const {
    // false for PVC XL A0 RevID==0x0, true from PVC XT A0 RevID==0x3==REVISION_B
    return (m_platformInfo.eProductFamily == IGFX_PVC && m_platformInfo.usRevId >= REVISION_B)
           || isCoreChildOf(IGFX_XE2_HPG_CORE);
  }

  bool hasFP64GlobalAtomicAdd() const {
    return (m_platformInfo.eProductFamily == IGFX_PVC && m_platformInfo.usRevId > REVISION_A0)
           || isCoreChildOf(IGFX_XE2_HPG_CORE);
  }

  bool supports16BitLdMcs() const { return isProductChildOf(IGFX_XE_HP_SDV) && IGC_IS_FLAG_ENABLED(Enable16BitLDMCS); }

  bool supportsGather4PO() const { return !isProductChildOf(IGFX_XE_HP_SDV); }

  // Typed read supports all renderable image formats, no image data conversion is
  // required.
  bool typedReadSupportsAllRenderableFormats() const {
    bool isChildOfDG2B0 = SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_B0);
    bool isChildOfDG2C0 = SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_C0);
    bool isDG2G11Config = GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID);
    bool isDG2G12Config = GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID);

    if ((m_platformInfo.eProductFamily == IGFX_DG2 && isChildOfDG2C0) ||
        (m_platformInfo.eProductFamily == IGFX_DG2 && isDG2G11Config && isChildOfDG2B0) ||
        (m_platformInfo.eProductFamily == IGFX_DG2 && isDG2G12Config) ||
        (m_platformInfo.eProductFamily == IGFX_METEORLAKE) || (m_platformInfo.eProductFamily == IGFX_ARROWLAKE) ||
        (m_platformInfo.eRenderCoreFamily == IGFX_XE_HPC_CORE) || isCoreChildOf(IGFX_XE2_HPG_CORE)) {
      return IGC_IS_FLAG_DISABLED(ForceFormatConversionDG2Plus);
    }

    return false;
  }

  bool EnableNewTileYCheckDefault() const {
    if (isCoreChildOf(IGFX_XE3_CORE)) {
      return false;
    }
    return true;
  }

  bool EnableKeepTileYForFlattenedDefault() const { return false; }

  bool supportsWriteableMSAATextures() const { return isCoreChildOf(IGFX_XE3_CORE); }
  bool supportsVRT() const { return isCoreChildOf(IGFX_XE3_CORE); }

  bool supportsOutOfBoundsGrfAccess() const { return !isCoreChildOf(IGFX_XE3_CORE); }
  bool needsOutOfBoundsBuiltinChecks() const {
    return !supportsOutOfBoundsGrfAccess() && IGC_IS_FLAG_ENABLED(EnableOutOfBoundsBuiltinChecks);
  }
  bool needsWAForThreadsUtilization() const {
    return (m_platformInfo.eProductFamily == IGFX_DG2 || m_platformInfo.eProductFamily == IGFX_METEORLAKE ||
            m_platformInfo.eProductFamily == IGFX_ARROWLAKE);
  }

  bool supportDualSimd8PS() const {
    return IGC_IS_FLAG_ENABLED(EnableDualSIMD8) && (m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE);
  }

  bool hasDualSimd8Payload() const {
    return (m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE) && !isCoreChildOf(IGFX_XE2_HPG_CORE);
  }

  unsigned int getMaxMeshShaderThreads() const { return m_caps.MediaShaderThreads - 1; }

  bool supportDpaswInstruction() const { return hasFusedEU() && supportDpasInstruction(); }

  // This represents the max number of logical lanes available for RT,
  // so it is not dependent on compiled SIMD size or "PreferredRayTracingSIMDSize".
  unsigned getRTStackDSSMultiplier() const {
    IGC_ASSERT(supportRayTracing());
    if (hasEfficient64bEnabled() && IGC_IS_FLAG_DISABLED(DisableSWManagedStack)) {
      unsigned int numSyncDssRTStacks = getsyncNumDSSRTStacks();

      // 0 is special value used for backward compatibility.
      // If number of SyncStacks, programmed in GlobalData is 0, HW assumes
      // 2048 stacks are used.
      if (numSyncDssRTStacks == 0) {
        numSyncDssRTStacks = 2048;
      }

      IGC_ASSERT(numSyncDssRTStacks <= 4096);

      return numSyncDssRTStacks;
    } else
      return 2048;
  }

  unsigned int getsyncNumDSSRTStacks() const {
    if (hasEfficient64bEnabled() && IGC_IS_FLAG_DISABLED(DisableSWManagedStack)) {
      if (IGC_IS_FLAG_SET(SWManagedStackNumStacks)) {
        return IGC_GET_FLAG_VALUE(SWManagedStackNumStacks);
      }

      return GetGTSystemInfo().MaxEuPerSubSlice * (GetGTSystemInfo().ThreadCount / GetGTSystemInfo().EUCount) *
             getMaxSimdSize();
    }

    return 0;
  }

  // Max number of hw threads for each workgroup
  unsigned int getMaxNumberHWThreadForEachWG() const {
    if (m_platformInfo.eRenderCoreFamily < IGFX_GEN12_CORE) {
      // Each WG is dispatched into one subslice for GEN11 and before
      return getMaxNumberThreadPerSubslice();
    } else if (m_platformInfo.eRenderCoreFamily <= IGFX_XE_HPC_CORE) {
      // Each WG is dispatched into one DSS which has 2 Subslices for Gen12 and above
      if (getWATable().Wa_1609337546 || getWATable().Wa_1609337769) {
        return 64;
      } else {
        return getMaxNumberThreadPerSubslice() * 2;
      }
    } else if (m_platformInfo.eRenderCoreFamily == IGFX_XE2_HPG_CORE) {
      // Each WG is dispatched into one subslice
      return getMaxNumberThreadPerSubslice();
    } else if (m_platformInfo.eRenderCoreFamily == IGFX_XE3_CORE) {
      return getMaxNumberThreadPerSubslice();
    } else if ((m_platformInfo.eRenderCoreFamily == IGFX_XE3P_CORE)
    ) {
      return getMaxNumberThreadPerSubslice();
    } else {
      IGC_ASSERT_MESSAGE(0, "Unsupported platform!");
    }
    return 0;
  }

  uint32_t getGRFSize() const { return isCoreChildOf(IGFX_XE_HPC_CORE) ? 64 : 32; }

  uint32_t getMaxNumGRF(ShaderType type) const {
    if (hasEfficient64bEnabled() &&
        isProductChildOf(IGFX_CRI)) {
      return (type == ShaderType::HULL_SHADER) ? 256 : 512;
    } else if (supports512GRFPerThread() && type == ShaderType::OPENCL_SHADER) {
      return 512;
    } else if (supportsVRT()) {
      return 256;
    } else {
      return 128;
    }
  }

  uint32_t getMinNumGRF() const { return supportsVRT() ? 32 : 128; }

  uint32_t getInlineDataSize() const {
    if (!supportInlineData())
      return 0;
    if (hasEfficient64bEnabled())
      return 64;
    return 32;
  }

  bool hasFusedEU() const {
    return m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE && !isCoreChildOf(IGFX_XE_HPC_CORE);
  }

  bool requireCallWA() const {
    return IGC_IS_FLAG_ENABLED(EnableCallWA) && hasFusedEU() && (getWATable().Wa_14016243945 == false);
  }

  bool hasPartialEmuI64Enabled() const { return hasPartialInt64Support() && IGC_IS_FLAG_ENABLED(EnablePartialEmuI64); }


  bool matchImmOffsetsLSC() const {
    enum LscMatchImmMode {
      OFF = 0,
      IF_HW_SUPPORTED = 1,
      ON = 2,
    };
    auto immOffsetMode = (LscMatchImmMode)IGC_GET_FLAG_VALUE(LscImmOffsMatch);
    return hasLSC() && (immOffsetMode >= ON || (immOffsetMode == IF_HW_SUPPORTED && isCoreChildOf(IGFX_XE2_HPG_CORE)));
  }
  bool supportStatefulScaleFolding() const {
    return hasEfficient64bEnabled() && IGC_IS_FLAG_ENABLED(EnableStatefulScaleFolding);
  }

  bool WaCubeHFPrecisionBug() const { return m_WaTable.Wa_18012201914 != 0; }

  // The max size in bytes of the scratch space per thread.
  //   XeHP_SDV and above are for each physical thread: 256k.
  //   TGLLP and below are for each FFTID: 2M.
  uint32_t maxPerThreadScratchSpace(
      // This is a temporary solution for the enabling.
      // It should be removed when all UMDs implement
      // support for 16MB scratch.
      bool sixteenMBPerThreadScratchSpace) const {
    if (IGC_IS_FLAG_ENABLED(MaxPerThreadScratchSpaceOverride)) {
      return IGC_GET_FLAG_VALUE(MaxPerThreadScratchSpaceOverride);
    }
    if (sixteenMBPerThreadScratchSpace && has16MBPerThreadScratchSpace())
      return 0x1000000;

    return hasScratchSurface() ? 0x40000 : 0x200000;
  }

  bool enableSpillCompressionCheckDefault() const {
    bool bEnabled = false;
    return bEnabled;
  }

  bool supportAIParameterCombiningWithLODBiasEnabled() const {
    return IGC_IS_FLAG_ENABLED(EnableAIParameterCombiningWithLODBias) &&
           ((m_platformInfo.eProductFamily == IGFX_DG2 && SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_B0)) ||
            GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID) || GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID) ||
            m_platformInfo.eProductFamily == IGFX_METEORLAKE || m_platformInfo.eProductFamily == IGFX_ARROWLAKE ||
            isCoreChildOf(IGFX_XE2_HPG_CORE));
  }

  bool useScratchSpaceForOCL() const {
    // Disable using scratch surface for private memory on XeHP_SDV
    // because it does not support byte-aligned (byte-scattered) messages.
    if (hasScratchSurface()) {
      return LSCEnabled() && IGC_IS_FLAG_ENABLED(EnableOCLScratchPrivateMemory) && isCoreChildOf(IGFX_XE_HPC_CORE);
    } else {
      return IGC_IS_FLAG_ENABLED(EnableOCLScratchPrivateMemory);
    }
  }

  // Check if byte ALU operations are well supported. If not, promote byte to i16/i32.
  bool supportByteALUOperation() const { return !isCoreChildOf(IGFX_XE_HPC_CORE); }

  // all the platforms which DONOT support 64 bit float operations
  bool hasNoFP64Inst() const {
    return m_platformInfo.eProductFamily == IGFX_ICELAKE_LP ||
           m_platformInfo.eProductFamily == IGFX_LAKEFIELD ||
           // m_platformInfo.eProductFamily == IGFX_ELKHARTLAKE || // same enum as JASPERLAKE
           m_platformInfo.eProductFamily == IGFX_JASPERLAKE || m_platformInfo.eProductFamily == IGFX_TIGERLAKE_LP ||
           m_platformInfo.eProductFamily == IGFX_ROCKETLAKE || m_platformInfo.eProductFamily == IGFX_ALDERLAKE_S ||
           m_platformInfo.eProductFamily == IGFX_ALDERLAKE_P || m_platformInfo.eProductFamily == IGFX_ALDERLAKE_N ||
           m_platformInfo.eProductFamily == IGFX_DG1 ||
           m_platformInfo.eProductFamily == IGFX_DG2;
  }

  // all the platforms which have correctly rounded macros (INVM, RSQRTM, MADM)
  bool hasCorrectlyRoundedMacros() const {
    if (isCoreChildOf(IGFX_XE2_HPG_CORE) && IGC_IS_FLAG_DISABLED(DisableCorrectlyRoundedMacros))
      return true;
    return (m_platformInfo.eProductFamily != IGFX_ICELAKE_LP &&
            m_platformInfo.eProductFamily != IGFX_LAKEFIELD && m_platformInfo.eProductFamily != IGFX_JASPERLAKE &&
            m_platformInfo.eProductFamily != IGFX_TIGERLAKE_LP && m_platformInfo.eProductFamily != IGFX_ROCKETLAKE &&
            m_platformInfo.eProductFamily != IGFX_DG1 && m_platformInfo.eProductFamily != IGFX_ALDERLAKE_S &&
            m_platformInfo.eProductFamily != IGFX_ALDERLAKE_P && m_platformInfo.eProductFamily != IGFX_ALDERLAKE_N &&
            m_platformInfo.eProductFamily != IGFX_DG2 && m_platformInfo.eRenderCoreFamily != IGFX_XE3_CORE &&
            m_platformInfo.eRenderCoreFamily != IGFX_XE3P_CORE &&
            m_platformInfo.eProductFamily != IGFX_METEORLAKE) &&
           m_platformInfo.eProductFamily != IGFX_ARROWLAKE && m_platformInfo.eProductFamily != IGFX_BMG &&
           m_platformInfo.eProductFamily != IGFX_LUNARLAKE;
  }

  // Has 64bit support but use 32bit for perf reasons
  bool preferFP32IntDivRemEmu() const {
    return (m_platformInfo.eProductFamily == IGFX_METEORLAKE || m_platformInfo.eProductFamily == IGFX_ARROWLAKE ||
            isCoreChildOf(IGFX_XE2_HPG_CORE));
  }

  // Platforms that haven't HW support for FP64 operations
  // and can emulate double operations
  bool emulateFP64ForPlatformsWithoutHWSupport() const { return m_platformInfo.eProductFamily == IGFX_DG2; }

  bool supportMixMode() const {
    return IGC_IS_FLAG_ENABLED(ForceMixMode) ||
           (IGC_IS_FLAG_DISABLED(DisableMixMode) &&
            (m_platformInfo.eProductFamily == IGFX_CHERRYVIEW || m_platformInfo.eProductFamily == IGFX_DG1 ||
             m_platformInfo.eProductFamily == IGFX_TIGERLAKE_LP || m_platformInfo.eProductFamily == IGFX_ROCKETLAKE ||
             m_platformInfo.eRenderCoreFamily == IGFX_GEN12LP_CORE ||
             m_platformInfo.eProductFamily == IGFX_ALDERLAKE_S || m_platformInfo.eProductFamily == IGFX_ALDERLAKE_P ||
             m_platformInfo.eProductFamily == IGFX_ALDERLAKE_N || m_platformInfo.eRenderCoreFamily == IGFX_GEN9_CORE ||
             m_platformInfo.eRenderCoreFamily == IGFX_GEN10_CORE));
  }

  bool supportsStaticRegSharing() const {
    // Static register sharing with multiple number of threads per EU supported on:
    return ((isProductChildOf(IGFX_XE_HP_SDV) && !WaDisableStaticRegSharing()) ||
            IGC_IS_FLAG_ENABLED(ForceSupportsStaticRegSharing));
  }

  bool DSPrimitiveIDPayloadPhaseCanBeSkipped() const {
    bool isFromFamilyWhereSkippable =
        m_platformInfo.eProductFamily == IGFX_ALDERLAKE_S || m_platformInfo.eProductFamily == IGFX_ALDERLAKE_P ||
        m_platformInfo.eProductFamily == IGFX_ALDERLAKE_N || m_platformInfo.eProductFamily == IGFX_TIGERLAKE_LP ||
        m_platformInfo.eProductFamily == IGFX_ROCKETLAKE || m_platformInfo.eProductFamily == IGFX_DG1;
    return (IGC_IS_FLAG_ENABLED(EnablePostCullPatchFIFOLP) && m_platformInfo.eRenderCoreFamily >= IGFX_GEN12_CORE &&
            isFromFamilyWhereSkippable) ||
           (IGC_IS_FLAG_ENABLED(EnablePostCullPatchFIFOHP) && isProductChildOf(IGFX_XE_HP_SDV));
  }

  bool emulateByteScraterMsgForSS() const {
    return IGC_IS_FLAG_ENABLED(EnableUntypedSurfRWofSS) && isProductChildOf(IGFX_XE_HP_SDV) && !hasLSC();
  }

  bool has64BMediaBlockRW() const {
    bool hasBlockRW = (IGC_IS_FLAG_ENABLED(Enable64BMediaBlockRW) && isProductChildOf(IGFX_XE_HP_SDV));
    hasBlockRW = hasBlockRW || isCoreChildOf(IGFX_XE2_HPG_CORE);
    return hasBlockRW;
  }

  int getBSOLocInExtDescriptor() const { return support26BitBSOFormat() ? 6 : 12; }

  bool NeedsHDCFenceBeforeEOTInPixelShader() const { return m_WaTable.Wa_1807084924 != 0; }

  bool canFuseTypedWrite() const {
    bool isChildOfDG2B0 = SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_B0);
    bool isChildOfDG2C0 = SI_WA_FROM(m_platformInfo.usRevId, ACM_G10_GT_REV_ID_C0);
    bool isDG2G11Config = GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID);
    bool isDG2G12Config = GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID);
    bool canFuse = (m_platformInfo.eProductFamily == IGFX_METEORLAKE) ||
                   (m_platformInfo.eProductFamily == IGFX_ARROWLAKE) ||
                   (m_platformInfo.eProductFamily == IGFX_DG2 && isChildOfDG2C0) ||
                   (m_platformInfo.eProductFamily == IGFX_DG2 && isDG2G11Config && isChildOfDG2B0) ||
                   (m_platformInfo.eProductFamily == IGFX_DG2 && isDG2G12Config);
    return (IGC_IS_FLAG_ENABLED(FuseTypedWrite) && canFuse);
  }

  bool enableSetDefaultTileYWalk() const {
    if (IGC_IS_FLAG_ENABLED(EnableTileYForExperiments))
      return true;

    // disable it on DG2 G11
    if (GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID))
      return false;

    return (supportHWGenerateTID());
  }

  // max block size for legacy OWord block messages
  uint32_t getMaxBlockMsgSize(bool isSLM) const {
    if (isSLM) {
      if (m_WaTable.Wa_14010875903) {
        return 64;
      }
      if (has16OWSLMBlockRW()) {
        return 256;
      }
    }
    return 128;
  }

  bool hasLSCUrbMessage() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }

  bool hasSampleMlodMessage() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }

  bool forceSamplerHeader() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }

  bool hasDualKSPPS() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }

  bool hasLSCSamplerRouting() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }

  bool hasBarrierControlFlowOpt() const {
    bool enabled = hasLSC() && IGC_IS_FLAG_ENABLED(EnableLSCFence) && isCoreChildOf(IGFX_XE2_HPG_CORE);

    return enabled;
  }

  bool needsLocalScopeEvictTGM() const {
    return
            true;
  }

  bool needWaSamplerNoMask() const { return m_WaTable.Wa_22011157800 && !IGC_IS_FLAG_DISABLED(DiableWaSamplerNoMask); }

  bool hasSlowSameSBIDLoad() const {
    bool bYes = false;
    bYes = isCoreChildOf(IGFX_XE_HPG_CORE) && !isCoreChildOf(IGFX_XE3P_CORE);

    return bYes && !needWaSamplerNoMask();
  }

  bool canDoMultipleLineMOVOpt() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }

  bool supportStochasticLod() const {
    return isCoreChildOf(IGFX_XE2_HPG_CORE);
  }

  bool supportsProgrammableOffsets() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }
  bool supportsProgrammableOffsetsSampleBCAndSampleDC() const { return isCoreChildOf(IGFX_XE3P_CORE); }

  bool supportsRayQueryThrottling() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }

  bool enableRayQueryThrottling(bool enableByDefault) const {
    if (!supportsRayQueryThrottling())
      return false;

    if (IGC_IS_FLAG_SET(OverrideRayQueryThrottling))
      return IGC_GET_FLAG_VALUE(OverrideRayQueryThrottling);

    if (isCoreChildOf(IGFX_XE3P_CORE))
      return true;

    return enableByDefault;
  }

  bool isSWSubTriangleOpacityCullingEmulationEnabled() const {
    return (isCoreChildOf(IGFX_XE3_CORE) &&
            IGC_IS_FLAG_DISABLED(DisableSWSubTriangleOpacityCullingEmulation)
    );
  }

  bool isRayQueryReturnOptimizationEnabled() const {
    return (isCoreChildOf(IGFX_XE2_HPG_CORE) && !getWATable().Wa_14018117913 &&
            IGC_IS_FLAG_DISABLED(DisableRayQueryReturnOptimization));
  }

  // ***** Below go accessor methods for testing WA data from WA_TABLE *****

  bool WaDoNotPushConstantsForAllPulledGSTopologies() const {
    return (m_platformInfo.eProductFamily == IGFX_BROADWELL) ||
           m_WaTable.WaDoNotPushConstantsForAllPulledGSTopologies != 0;
  }

  bool WaForceMinMaxGSThreadCount() const { return m_WaTable.WaForceMinMaxGSThreadCount != 0; }

  bool WaOCLEnableFMaxFMinPlusZero() const { return m_WaTable.WaOCLEnableFMaxFMinPlusZero != 0; }

  bool WaDisableSendsSrc0DstOverlap() const { return m_WaTable.WaDisableSendsSrc0DstOverlap != 0; }

  bool WaDisableEuBypass() const { return (m_WaTable.WaDisableEuBypassOnSimd16Float32 != 0); }

  bool WaDisableDSDualPatchMode() const { return m_WaTable.WaDisableDSDualPatchMode == 0; }

  bool WaDispatchGRFHWIssueInGSAndHSUnit() const { return m_WaTable.WaDispatchGRFHWIssueInGSAndHSUnit != 0; }

  bool WaSamplerResponseLengthMustBeGreaterThan1() const {
    return m_WaTable.WaSamplerResponseLengthMustBeGreaterThan1 != 0;
  }

  bool WaDisableDSPushConstantsInFusedDownModeWithOnlyTwoSubslices() const {
    return ((m_WaTable.WaDisableDSPushConstantsInFusedDownModeWithOnlyTwoSubslices) &&
            (m_GTSystemInfo.IsDynamicallyPopulated && m_GTSystemInfo.SubSliceCount == 2));
  }

  bool WaDisableVSPushConstantsInFusedDownModeWithOnlyTwoSubslices() const {
    return ((m_WaTable.WaDisableVSPushConstantsInFusedDownModeWithOnlyTwoSubslices) &&
            (m_GTSystemInfo.IsDynamicallyPopulated && m_GTSystemInfo.SubSliceCount == 2));
  }

  bool WaForceCB0ToBeZeroWhenSendingPC() const { return m_WaTable.WaForceCB0ToBeZeroWhenSendingPC != 0; }

  bool WaConservativeRasterization() const {
    return (m_WaTable.WaConservativeRasterization != 0 && IGC_IS_FLAG_ENABLED(ApplyConservativeRastWAHeader));
  }

  bool WaReturnZeroforRTReadOutsidePrimitive() const { return m_WaTable.WaReturnZeroforRTReadOutsidePrimitive != 0; }

  bool WaFixInnerCoverageWithSampleMask() const { return m_WaTable.Wa_220856683 != 0; }

  bool WaOverwriteFFID() const { return m_WaTable.Wa_1409460247 != 0; }

  bool WaDisableStaticRegSharing() const { return m_WaTable.Wa_14012688715 != 0; }

  bool WaDisablePrimitiveReplicationWithCPS() const {
    return m_WaTable.Wa_18013852970 != 0;
  }

  bool supportSystemFence() const {
    return hasLSC() && m_platformInfo.eProductFamily != IGFX_DG2 && m_platformInfo.eProductFamily != IGFX_METEORLAKE &&
           m_platformInfo.eProductFamily != IGFX_ARROWLAKE;
  }

  bool WaGeoShaderURBAllocReduction() const { return m_WaTable.Wa_18012660806 != 0; }

  bool WaDisableSendSrcDstOverlap() const {
    return (!IGC_IS_FLAG_ENABLED(DisableSendSrcDstOverlapWA)) &&
           (m_SkuTable.FtrWddm2Svm != 0 || m_platformInfo.eRenderCoreFamily == IGFX_GEN10_CORE ||
            m_platformInfo.eRenderCoreFamily == IGFX_GEN11_CORE ||
            (m_platformInfo.eProductFamily >= IGFX_PVC && m_platformInfo.eProductFamily <= IGFX_ARROWLAKE));
  }

  bool WaInsertHDCFenceBeforeEOTWhenSparseAliasedResources() const { return m_WaTable.Wa_1807084924 != 0; }

  bool WaDisableSampleLz() const { return (IGC_IS_FLAG_DISABLED(DisableWaSampleLZ) && m_WaTable.Wa_14013297064); }

  bool WaEnableA64WA() const {
    if (IGC_IS_FLAG_ENABLED(EnableA64WA) && m_WaTable.Wa_14010595310) {
      return true;
    }
    return false;
  }

  // Only enable this WA for TGLLP+ because, in pre TGLLP projects, smov was replaced with two instructions which caused
  // performance penalty.
  bool enableMultiGRFAccessWA() const { return (m_platformInfo.eProductFamily >= IGFX_TIGERLAKE_LP); }

  // Return true if platform has structured control-flow instructions and IGC wants to use them.
  bool hasSCF() const {
    // DG2 and PVC still has SCF, but igc will stop using them.
    return !isProductChildOf(IGFX_DG2);
  }

  const SCompilerHwCaps &GetCaps() { return m_caps; }

  bool supportHeaderRTW() const { return (!isCoreChildOf(IGFX_XE2_HPG_CORE)); }

  bool preemptionSupported() const {
    if (isCoreChildOf(IGFX_XE_HPC_CORE) && !isCoreChildOf(IGFX_XE2_HPG_CORE))
      return false;

    return GetPlatformFamily() >= IGFX_GEN9_CORE;
  }

  // platform natively not support DW-DW multiply
  bool noNativeDwordMulSupport() const {
    return m_platformInfo.eProductFamily == IGFX_BROXTON || m_platformInfo.eProductFamily == IGFX_GEMINILAKE ||
           m_platformInfo.eProductFamily == IGFX_DG2 || m_platformInfo.eProductFamily == IGFX_METEORLAKE ||
           m_platformInfo.eProductFamily == IGFX_ARROWLAKE || GetPlatformFamily() == IGFX_GEN11_CORE ||
           GetPlatformFamily() == IGFX_GEN12LP_CORE;
  }

  unsigned getURBFullWriteMinGranularity() const {
    unsigned overrideValue = IGC_GET_FLAG_VALUE(SetURBFullWriteGranularity);
    if (overrideValue == 16 || overrideValue == 32) {
      return overrideValue;
    }
    return isProductChildOf(IGFX_XE_HP_SDV) ? 16 : 32; // in bytes
  }

  unsigned forceQwAtSrc0ForQwShlWA() const {
    // PVC XT A0: RevID==0X3==REVISION_B
    return (m_platformInfo.eProductFamily == IGFX_PVC && m_platformInfo.usRevId == REVISION_B);
  }


  bool hasSIMD8Support() const {
    if (isCoreChildOf(IGFX_XE2_HPG_CORE))
      return false;
    return !(m_platformInfo.eRenderCoreFamily == IGFX_XE_HPC_CORE);
  }

  bool hasThreadPauseSupport() const {
    if (isCoreChildOf(IGFX_XE_HPC_CORE))
      return false;
    return true;
  }

  bool WaDisableD64ScratchMessage() const { return getWATable().Wa_15010203763 != 0; }

  bool supportLargeGRF() const { return (isCoreChildOf(IGFX_XE_HPG_CORE) && !isCoreChildOf(IGFX_XE3_CORE)); }

  bool supportCheckCSThreadsLimit() const { return (m_platformInfo.eRenderCoreFamily == IGFX_XE2_HPG_CORE); }
  bool supportTriggerLargeGRFRetry() const { return (m_platformInfo.eRenderCoreFamily == IGFX_XE2_HPG_CORE); }
  bool supports512GRFPerThread() const { return isCoreChildOf(IGFX_XE3P_CORE); }
  bool supportsQuantumDispatch() const { return isCoreChildOf(IGFX_XE3P_CORE); }

  bool supportsPureBF() const {
    // Maps to vISA's "supportPureBF"
    return isCoreChildOf(IGFX_XE3P_CORE);
  }

  bool EnableCSWalkerPass() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }

  bool limitedBCR() const {
    return m_platformInfo.eProductFamily == IGFX_DG2 &&
           (GFX_IS_DG2_G11_CONFIG(m_platformInfo.usDeviceID) || GFX_IS_DG2_G12_CONFIG(m_platformInfo.usDeviceID));
  }

  uint32_t getMaxAddressedHWThreads() const {
    // TODO Calculate the value
    if (isCoreChildOf(IGFX_XE2_HPG_CORE))
      return 8192;
    return 4096;
  }

  bool supportsNumberOfBariers() const { return isProductChildOf(IGFX_DG2); }

  uint32_t getVISAABIVersion() const {
    if (isCoreChildOf(IGFX_XE3P_CORE))
      return 3;
    return 2;
  }

  bool supportsLoadStatusMessages() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }

  bool supportsNonDefaultLSCCacheSetting() const {
    return isCoreChildOf(IGFX_XE2_HPG_CORE);
  }

  bool hasNewLSCCacheEncoding() const {
    return isCoreChildOf(IGFX_XE3P_CORE);
  }
  bool isSupportedLSCCacheControlsEnum(LSC_L1_L3_CC l1l3cc, bool isLoad) const {
    if (hasNewLSCCacheEncoding())
      return true;
    if (isLoad) {
      switch (l1l3cc) {
      default:
        break;
      case LSC_L1UC_L3CC:
      case LSC_L1C_L3CC:
      case LSC_L1IAR_L3IAR:
        return isCoreChildOf(IGFX_XE2_HPG_CORE);
      }
    }
    return true;
  }

  unsigned int roundUpTgsmSize(DWORD size) const {
    const DWORD blockSize = getSharedLocalMemoryBlockSize() ? getSharedLocalMemoryBlockSize() : sizeof(KILOBYTE);
    size = iSTD::Round(size, blockSize) / blockSize;
    if (size > 16) {
      if (isCoreChildOf(IGFX_XE3P_CORE) && size > 256) {
        // 320K does not follow Xe2 pattern
        return (size > 320) ? 384 * blockSize : 320 * blockSize;
      }
      if (isCoreChildOf(IGFX_XE2_HPG_CORE)) {
        const DWORD roundSize = iSTD::RoundPower2(size) / 4;
        return iSTD::Round(size, roundSize) * blockSize;
      }
    }
    return iSTD::RoundPower2(size) * blockSize;
  }

  bool supports2dBlockTranspose64ByteWidth() const { return isCoreChildOf(IGFX_XE3P_CORE); }

  bool supportsReadStateInfo() const {
    return hasEfficient64bEnabled() && (IGC_IS_FLAG_ENABLED(EnableReadStateToA64Read) || m_WaTable.Wa_14025275057);
  }

  bool supportsFp4Int4Upsampling() const { return isCoreChildOf(IGFX_XE3P_CORE); }


  bool supportsRayTracingExtendedCacheControl() const {
    return isProductChildOf(IGFX_CRI) && hasEfficient64bEnabled() &&
           IGC_IS_FLAG_DISABLED(DisableRayTracingExtendedCacheControl);
  }


  bool supportsOverfetch() const {
    return isProductChildOf(IGFX_CRI);
  }

  bool supportsNativeHyperbolicTan() const { return isCoreChildOf(IGFX_XE3P_CORE); }

  bool supportsNativeTanh() const { return supportsNativeHyperbolicTan() && IGC_IS_FLAG_ENABLED(EnableNativeTanh); }

  bool supportsNativeSigmoid() const { return isCoreChildOf(IGFX_XE3P_CORE); }

  bool supportsNativeSinCos() const {
    return isProductChildOf(IGFX_CRI) && IGC_IS_FLAG_ENABLED(EnableNativeSinCos);
  }

  bool hasAccurateLog2() const {
    return isProductChildOf(IGFX_CRI);
  }


  unsigned getRayTracingTileXDim1D() const {
    if (isCoreChildOf(IGFX_XE2_HPG_CORE)) {
      return 512;
    }
    return 256;
  }

  unsigned getRayTracingTileYDim1D() const { return 1; }

  unsigned getRayTracingTileXDim2D() const {
    if (isCoreChildOf(IGFX_XE2_HPG_CORE))
      return 8;
    else
      return 32;
  }

  unsigned getRayTracingTileYDim2D() const {
    if (isCoreChildOf(IGFX_XE2_HPG_CORE))
      return 128;
    else
      return 4;
  }

  bool preferLSCCache() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }
  bool canCachePartialWrites() const { return isCoreChildOf(IGFX_XE2_HPG_CORE); }

  bool usesDynamicPolyPackingPolicies() const {
    return isCoreChildOf(IGFX_XE3_CORE) && IGC_IS_FLAG_DISABLED(DisableDynamicPolyPackingPolicies);
  }

  bool allowDivergentControlFlowRayQueryCheckRelease() const { return m_WaTable.Wa_22019804511 == 0; }

  bool allowProceedBasedApproachForRayQueryDynamicRayManagementMechanism() const {
    return IGC_IS_FLAG_DISABLED(DisableProceedBasedApproachForRayQueryDynamicRayManagementMechanism);
  }

  bool allowsMoviForType(VISA_Type type) const { return (type == ISA_TYPE_UD || type == ISA_TYPE_D); }

  bool enableLscSamplerRouting() const {
    return isCoreChildOf(IGFX_XE3_CORE);
  }

  bool enableReplaceAtomicFenceWithSourceValue() const {
    return !isCoreChildOf(IGFX_XE3P_CORE) && IGC_IS_FLAG_ENABLED(ReplaceAtomicFenceWithSourceValue);
  }

  uint16_t getNumAddrRegisters() const {
    return 16;
  }
};
} // namespace IGC
