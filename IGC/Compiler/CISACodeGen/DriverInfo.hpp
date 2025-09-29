/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/igc_regkeys.hpp"
#include "common/Types.hpp"
#include "inc/common/igfxfmid.h"
#include "CommonMacros.h"

/*
This provides hook to query whether a feature is supported by the runtime we are compiling for
This file has default value, then each adapter can overload any of the query to tell the backend
what it supports and what it doesn't. This also implements some workaround in case some API
or driver doesn't support something
*/

namespace IGC {

class CDriverInfo {
public:
  /// The driver implements the WA using constant buffer 2 for NOS constants instead of 0
  virtual bool implementPushConstantWA() const { return false; }

  /// Driver supports Simple Push Mechanism only.
  virtual bool SupportsSimplePushOnly() const { return false; }

  /// Driver supports Gather Constant Mechanism only.
  virtual bool SupportsGatherConstantOnly() const { return false; }

  /// Driver supports resource streamer if HW supportes it, otherwise simple push
  virtual bool SupportsHWResourceStreameAndSimplePush() const { return false; }

  /// Driver supports dynamic uniform buffers.
  virtual bool SupportsDynamicUniformBuffers() const { return false; }

  /// Is any special metadata translation required
  virtual bool NeedsMetadataTranslation() const { return false; }

  /// Do we need to break down the fmuladd
  virtual bool NeedsBreakdownMulAdd() const { return false; }

  /// The driver supports using scratch space to store the private memory
  virtual bool supportsScratchSpacePrivateMemory() const { return true; }

  /// The driver supports using stateless space to store the private memory
  /// Driver must be able to use at least one way to store the private memory: either "scratch space" or "stateless
  /// space" and by default, driver only supports one of them. NOTE: This method should only be used for XeHP and above
  /// to avoid changes to legacy GENs
  ///         And this is the only place telling if one API supports statelesspvtmem or not.
  ///         If this API doesn't support statelesspvtmem, IGC will error out if pvtmemusage > 256k in
  ///         PrivateMemoryResolution
  virtual bool supportsStatelessSpacePrivateMemory() const { return !supportsScratchSpacePrivateMemory(); }

  /// The driver requires to align each entry (a workgroup item) of private scratch memory in a stateless
  /// buffer.
  virtual bool requiresPowerOfTwoStatelessSpacePrivateMemorySize() const { return false; }

  /// The driver supports splitting up scratch memory space into two areas:
  /// - private scratch memory space: non-promoted alloca instructions (early allocated scratch
  ///   memory space based on llvm IR)
  /// - spill/fill and Gtpin scratch memory space: (late allocated scratch memory space based
  ///   registry allocation)
  virtual bool supportsSeparatingSpillAndPrivateScratchMemorySpace() const {
    return IGC_IS_FLAG_ENABLED(SeparateSpillPvtScratchSpace);
  }

  /// The driver Uses special states to push constants beyond index 256
  virtual bool Uses3DSTATE_DX9_CONSTANT() const { return false; }

  /// The driver uses typed or untyped constant buffers (for ld_raw vs sampler)
  virtual bool UsesTypedConstantBuffers3D() const { return true; }

  /// The driver uses typed or untyped constant buffers (for ld_raw vs sampler)
  virtual bool UsesTypedConstantBuffersGPGPU() const { return true; }

  /// Overwrite UsesTypedConstantBuffers3D() and UsesTypedConstantBuffersGPGPU()
  /// for bindless buffers only.
  virtual bool ForceUntypedBindlessConstantBuffers() const { return false; }

  /// The driver uses sparse aliased residency
  virtual bool UsesSparseAliasedResidency() const { return false; }

  /// The driver doesn't clear the vertex header so it needs to be done in the compiler
  virtual bool NeedClearVertexHeader() const { return false; }

  /// Do Fastest Stage1 only for 3D
  virtual bool SupportFastestStage1() const { return true; }

  /// do code sinking before CFGSimplification, helps some workloads
  virtual bool CodeSinkingBeforeCFGSimplification() const { return false; }

  /// allow executing constant buffer on the CPU
  virtual bool AllowGenUpdateCB(ShaderType shaderType) const {
    IGC_UNUSED(shaderType);
    return false;
  }

  /// The driver implements single instance vertex dispatch feature
  virtual bool SupportsSingleInstanceVertexDispatch() const { return false; }

  // Allow branch swapping for better Nan perf
  virtual bool BranchSwapping() const { return false; }

  /// Allow propagation up-converstion of half if it can generate better code
  virtual bool AllowUnsafeHalf() const { return true; }

  /// Allow send fusion (Some API have perf regressions, temp use to turn it off)
  virtual bool AllowSendFusion() const { return true; }

  /// Supports more than 16 samplers
  virtual bool SupportMoreThan16Samplers() const { return false; }

  /// API supports IEEE min/max
  virtual bool SupportsIEEEMinMax() const { return false; }

  virtual bool NeedCountSROA() const { return false; }

  /// Can we always contract mul and add
  virtual bool NeedCheckContractionAllowed() const { return false; }

  /// The API generates load/store of doubles which needs to be broken down
  virtual bool HasDoubleLoadStore() const { return false; }

  /// Needs emulation of 64bits instructions
  virtual bool NeedI64BitDivRem() const { return false; }

  /// Return true if IGC needs FP64 emulation. (Valid if platform has no double inst.)
  virtual bool NeedFP64(PRODUCT_FAMILY productFamily) const {
    IGC_UNUSED(productFamily);
    return false;
  }

  /// Needs fp64 to fp16 conversion emulation
  virtual bool NeedFP64toFP16Conv() const { return false; }

  /// Needs IEEE fp64 div/sqrt
  virtual bool NeedFP64DivSqrt() const { return false; }

  /// Must support of f32 IEEE divide (also sqrt)
  virtual bool NeedIEEESPDiv() const { return false; }

  /// Has memcpy/memset intrinsic
  virtual bool HasMemoryIntrinsics() const { return false; }

  /// Has load store not natively supported
  virtual bool HasNonNativeLoadStore() const { return false; }

  /// Need lowering global inlined constant buffers
  virtual bool NeedLoweringInlinedConstants() const { return false; }

  /// Turn on type demotion, not tested on all APIs
  virtual bool benefitFromTypeDemotion() const { return false; }

  /// Turn on type rematerialization of flag register, not tested on all APIs
  virtual bool benefitFromPreRARematFlag() const { return false; }

  /// add extra optimization passes after AlwaysInlinerPass to support two phase inlining
  virtual bool NeedExtraPassesAfterAlwaysInlinerPass() const { return false; }

  /// Turn on vISA pre-RA scheduler. Not tested on all APIs
  virtual bool enableVISAPreRAScheduler() const { return false; }

  /// Turn on vISA pre-RA scheduler for retry
  virtual bool enableVISAPreRASchedulerForRetry() const { return false; }

  /// Configure vISA pre-RA scheduler. Not tested on all APIs
  virtual unsigned getVISAPreRASchedulerCtrl() const { return 4; }

  /// VISA pre-RA scheduler configuration for kernels with dpas.
  virtual unsigned getVISAPreRASchedulerCtrlDpas() const { return 4; }

  /// Make sure optimization are consistent to avoid Z-fighting issue
  virtual bool PreventZFighting() const { return false; }

  /// Force enabling SIMD32 in case we exepct latency problem. Helps some workloads
  virtual bool AlwaysEnableSimd32() const { return false; }

  /// Driver supports promoting buffers to bindful
  virtual bool SupportsStatelessToStatefulBufferTransformation() const { return false; }

  /// Need emulation of 64bits type for HW not supporting it natively
  virtual bool Enable64BitEmu() const { return false; }

  /// In some cases several BTI may alias
  virtual bool DisableDpSendReordering() const { return false; }

  /// Driver uses HW alt math mode, this cause floating point operations to behave differently
  virtual bool UseALTMode() const { return false; }

  /// Whether the driver supports blend to fill opt
  virtual bool SupportBlendToFillOpt() const { return false; }

  /// Need to know if the driver can accept more than one SIMD mode for compute shaders
  virtual bool sendMultipleSIMDModes() const { return false; }

  /// pick behavior whether we need to keep discarded helper pixels to calculate
  /// gradient correctly for sampler or we need to force early out discarded pixels
  virtual bool KeepDiscardHelperPixels() const { return false; }

  // Choose to support parsing inlined asm instructions on specific platforms
  virtual bool SupportInlineAssembly() const { return false; }

  /// support predicate add pattern match
  virtual bool SupportMatchPredAdd() const { return false; }

  /// Support passing globally accessed pointers implicitly to callees using argument registers
  virtual bool SupportGlobalStackArgs() const { return false; }

  /// Adjust adapter to adjust the loop unrolling threshold
  virtual unsigned int GetLoopUnrollThreshold() const { return 4000; }

  /// Need HDC memory fence when raster order views are used
  virtual bool NeedUavPixelSyncAddedInPSLowering() const { return true; }

  // ----------------------------------------------------------------------
  // Below are workaround for bugs in front end or IGC will be removed once
  // the bugs are fixed

  /// Need workaround for A32 messages used along with A64
  virtual bool NeedWAToTransformA32MessagesToA64() const { return false; }

  /// disable mad in Vertex shader to avoid ZFigthing issues
  virtual bool DisabeMatchMad() const { return false; }

  /// Some FE sends SLM pointers in DWORD units
  virtual bool WASLMPointersDwordUnit() const { return false; }

  /// Custom pass haven't been tested on all APIs
  virtual bool WADisableCustomPass() const { return false; }

  /// MemOpt2ForOCL pass not tested on all APIs
  virtual bool WAEnableMemOpt2ForOCL() const { return false; }

  /// disable some optimizations for front end which sends IR with unresolved NOS function when optimizing
  virtual bool WaNOSNotResolved() const { return false; }

  /// WA for APIs where frc generates a different precision than x - rndd(x) for small negative values
  /// Needs to switch to use fast math flags
  virtual bool DisableMatchFrcPatternMatch() const { return false; }

  /// Based on the type of inlined sampler we get we program different output.
  virtual bool ProgrammableBorderColorInCompute() const { return false; }

  /// WA for failures with HS with push constants
  virtual bool WaDisablePushConstantsForHS() const { return false; }

  /// WA for failures with push constants and no pushed attributes
  virtual bool WaDisablePushConstantsWithNoPushedAttributes() const { return false; }

  /// Check if we have to worry about stack overflow while recursing in loop analysis
  virtual bool HasSmallStack() const { return false; }

  /// Check if the stateful token is supported
  virtual bool SupportStatefulToken() const { return false; }

  /// Disables dual patch dispatch for APIs that don't use it
  virtual bool APIDisableDSDualPatchDispatch() const { return false; }

  /// WA to make sure scratch writes are globally observed before EOT
  virtual bool clearScratchWriteBeforeEOT() const { return false; }

  /// Should unaligned vectors be split before processing in EmitVISA
  virtual bool splitUnalignedVectors() const { return true; }

  /// Does not emit an error if recursive functions calls are detected.
  virtual bool AllowRecursion() const { return false; }

  /// Rounding mode used for DP emulated function, defaults to Round to nearest
  virtual unsigned DPEmulationRoundingMode() const { return 0; }

  /// Check for flushing denormals for DP emulated function
  virtual bool DPEmulationFlushDenorm() const { return false; }

  /// Check for flush to zero for DP emulated function
  virtual bool DPEmulationFlushToZero() const { return false; }

  // Maximum id that can be used by simple push constant buffers. The default is maximum unsigned int (no restriction)
  virtual unsigned int MaximumSimplePushBufferID() const { return std::numeric_limits<unsigned int>::max(); }

  /// Enables the use of inline data on XeHP_SDV+
  virtual bool UseInlineData() const { return false; }

  /// Use first VB to send vertex&base instance and second for draw index
  virtual bool UsesVertexBuffersToSendShaderDrawParameters() const { return false; }

  /// Use indirect payload in CS
  virtual bool UsesIndirectPayload() const { return true; }

  virtual bool SupportsDispatchGPGPUWalkerAlongYFirst() const { return true; }

  /// Check if integer mad is enabled
  virtual bool EnableIntegerMad() const { return false; }

  /// Respect per instruction 'contract' Fast-Math flag
  virtual bool RespectPerInstructionContractFlag() const { return false; }

  /// add shader hash code after EOT for debug purposes
  virtual bool EnableShaderDebugHashCodeInKernel() const { return false; }

  // The size of output printf buffer is 4 MB by default by agreement with Runtime.
  virtual uint32_t getPrintfBufferSize() const { return 4 * sizeof(MEGABYTE); }

  // Determines whether the PAYLOAD_HEADER implicit arg must be present
  virtual bool RequirePayloadHeader() const { return true; }

  virtual bool supportsAutoGRFSelection() const {
    return autoGRFSelection || IGC_IS_FLAG_ENABLED(ForceSupportsAutoGRFSelection);
  }
  virtual void setAutoGRFSelection(bool value) { autoGRFSelection = value; }
  virtual bool UseScratchSpaceForATSPlus() const { return false; }
       /// Enables HWGenerateThreadID from API level. To help debug, we must enable it from both API level AND IGC Core
       /// level.
  virtual bool SupportHWGenerateTID() const { return false; }
  // Enables the use of simple push constants when on platforms with local (device) memory
  virtual bool supportsSimplePushForLocalMem() const { return false; }
  // disable dual8 with discard
  virtual bool DisableDual8WithDiscard() const { return false; }
  // support force routing to HDC and LCS caching options
  virtual bool SupportForceRouteAndCache() const { return false; }
  // If enabled, IGC must provide the corresponding UMD info on how much
  // memory to allocate for the RTGlobals + global root signature.
  virtual bool supportsExpandedRTGlobals() const { return false; }
  // Enables the use of scratch space in raytracing shaders when possible
  virtual bool supportsRTScratchSpace() const { return false; }
  // Enables Raytracing printf
  virtual bool SupportsRTPrintf() const { return false; }
  // enables stateful accesses to the RTAsyncStack, SWHotZone, SWStack and RTSyncStack
  virtual bool supportsRaytracingStatefulAccesses() const { return false; }
  // To support this, the compiler output must be able to express a
  // raygen shader identifier with continuation KSPs after it.
  virtual bool supportsRaytracingContinuationPromotion() const { return false; }
  // To support this, UMD must flip the X and Y dimensions
  virtual bool supportsRaytracingDispatchComputeWalkerAlongYFirst() const { return false; }
  // Will the UMD patch the call stack handler with KSP pointers?
  virtual bool supportsCallStackHandlerPatching() const { return false; }
  // Support checkLocalIDs in WIA
  virtual bool supportWIALocalIDs() const { return false; }

  // Enable LSC on DG2 for the following:
  //   GenISAIntrinsic::GenISA_ldraw_indexed
  //   GenISAIntrinsic::GenISA_ldrawvector_indexed
  //   GenISAIntrinsic::GenISA_storeraw_indexed
  //   GenISAIntrinsic::GenISA_storerawvector_indexed
  // todo: remove when all APIs enable LSC
  virtual bool EnableLSCForLdRawAndStoreRawOnDG2() const { return false; }
  // Check SLM limit on compute shader to select SIMD8
  virtual bool SupportCSSLMLimit() const { return false; }
  virtual bool supportsSIMD32forCPS() const { return true; }

  // When dual-source blending is enabled, enable sending the
  // single-source RTW message (with data for the second color) after the
  // dual-source blending RTW message. The second message must be send
  // when the state of dual-source blending is not known at compile time.
  virtual bool sendSingleSourceRTWAfterDualSourceRTW() const { return true; }


  virtual bool needsRegisterAccessBoundsChecks() const {
    // Disabled by default, can be enabled via registry key.
    const IGC::TriboolFlag registerAccessBoundsCheckCtrl =
        static_cast<IGC::TriboolFlag>(IGC_GET_FLAG_VALUE(ForceRegisterAccessBoundsChecks));
    return registerAccessBoundsCheckCtrl == IGC::TriboolFlag::Enabled;
  }

  // Specifies alignment of indirect data
  virtual unsigned getCrossThreadDataAlignment() const { return 32; }

  // If enabled IGC must not hoist convergent instructions.
  virtual bool DisableConvergentInstructionsHoisting() const { return false; }

  // Each API can define its own preferred values for the spill threshold
  virtual unsigned getSIMD8_SpillThreshold() const { return IGC_GET_FLAG_VALUE(SIMD8_SpillThreshold); }
  virtual unsigned getSIMD16_SpillThreshold() const { return IGC_GET_FLAG_VALUE(SIMD16_SpillThreshold); }
  virtual unsigned getSIMD32_SpillThreshold() const { return IGC_GET_FLAG_VALUE(SIMD32_SpillThreshold); }
  virtual unsigned getCSSIMD16_SpillThreshold() const { return IGC_GET_FLAG_VALUE(CSSIMD16_SpillThreshold); }
  virtual unsigned getCSSIMD32_SpillThreshold() const { return IGC_GET_FLAG_VALUE(CSSIMD32_SpillThreshold); }

  virtual bool supportLscSamplerRouting() const { return true; }
  virtual bool supportBarrierControlFlowOptimization() const { return false; }
  virtual bool getLscStoresWithNonDefaultL1CacheControls() const { return true; }

  // Informs if the UMD understands atomic pull tile walk for raytracing
  virtual bool supportsAtomicPullSWTileWalk() const { return false; }
  virtual bool supportsDynamicPolyPackingPolicies() const { return true; }
  virtual bool supportsVRT() const { return true; }

  virtual bool supportsUniformPrivateMemorySpace() const { return false; }
  virtual uint32_t maxNumCoherenceHintBitsForReorderThread() const { return 0; }

  virtual bool UseNewTraceRayInlineLoweringInRaytracingShaders() const {
    return (IGC_GET_FLAG_VALUE(UseNewInlineRaytracing) & static_cast<uint32_t>(NewInlineRaytracingMask::RTShaders)) !=
           0;
  }
  virtual bool UseNewTraceRayInlineLoweringInNonRaytracingShaders() const {
    return (IGC_GET_FLAG_VALUE(UseNewInlineRaytracing) &
            static_cast<uint32_t>(NewInlineRaytracingMask::NonRTShaders)) != 0;
  }

protected:
  bool autoGRFSelection = false;
};

} // namespace IGC
