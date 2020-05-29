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
#include "common/igc_regkeys.hpp"
#include "common/Types.hpp"
#include "inc/common/igfxfmid.h"

/*
This provides hook to query whether a feature is supported by the runtime we are compiling for
This file has default value, then each adapter can overload any of the query to tell the backend
what it supports and what it doesn't. This also implements some workaround in case some API
or driver doesn't support something
*/

namespace IGC
{

    class CDriverInfo
    {
    public:
        /// The driver implements the WA using constant buffer 2 for NOS constants instead of 0
        virtual bool implementPushConstantWA() const { return false; }

        /// Driver supports Simple Push Mechanism only.
        virtual bool SupportsSimplePushOnly() const { return false; }

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
        /// Driver must be able to use at least one way to store the private memory: either "scratch space" or "stateless space"
        /// and by default, driver only supports one of them.
        virtual bool supportsStatelessSpacePrivateMemory() const { return !supportsScratchSpacePrivateMemory(); }

        /// The max size in bytes of the scratch space per thread.
        unsigned int maxPerThreadScratchSpace() const { return 2 * 1024 * 1024; }

        /// The driver Uses special states to push constants beyond index 256
        virtual bool Uses3DSTATE_DX9_CONSTANT() const { return false; }

        /// The driver uses typed or untyped constant buffers (for ld_raw vs sampler)
        virtual bool UsesTypedConstantBuffers3D() const { return true; }

        /// The driver uses typed constant buffers requiring byte address access.
        virtual bool UsesTypedConstantBuffersWithByteAddress() const { return false; }

        /// The driver uses typed or untyped constant buffers (for ld_raw vs sampler)
        virtual bool UsesTypedConstantBuffersGPGPU() const { return true; }

        /// The driver uses sparse aliased residency
        virtual bool UsesSparseAliasedResidency() const { return false; }

        /// The driver doesn't clear the vertex header so it needs to be done in the compiler
        virtual bool NeedClearVertexHeader() const { return false; }

        /// do code sinking before CFGSimplification, helps some workloads
        virtual bool CodeSinkingBeforeCFGSimplification() const { return false; }

        /// allow executing constant buffer on the CPU
        virtual bool AllowGenUpdateCB(ShaderType shaderType) const { return false; }

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

        /// Supports precise math
        virtual bool SupportsPreciseMath() const { return false; }

        virtual bool NeedCountSROA() const { return false; }

        /// Can we always contract mul and add
        virtual bool NeedCheckContractionAllowed() const { return false; }

        /// The API generates load/store of doubles which needs to be broken down
        virtual bool HasDoubleLoadStore() const { return false; }

        /// Needs emulation of 64bits instructions
        virtual bool NeedI64BitDivRem() const { return false; }

        /// Must support FP64
        virtual bool NeedFP64(PRODUCT_FAMILY productFamily) const { return false; }

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

        /// Configure vISA pre-RA scheduler. Not tested on all APIs
        virtual unsigned getVISAPreRASchedulerCtrl() const { return 4; }

        /// Turn on sampler clustering. Hopefully VISA PreRA scheduler with latency hiding can replace it.
        virtual bool enableSampleClustering() const { return true; }

        /// Make sure optimization are consistent to avoid Z-fighting issue
        virtual bool PreventZFighting() const { return false; }

        /// Force enabling SIMD32 in case we exepct latency problem. Helps some workloads
        virtual bool AlwaysEnableSimd32() const { return false; }

        /// Driver supports promoting buffers to bindful
        virtual bool SupportsStatelessToStatefullBufferTransformation() const { return false; }

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
        virtual bool SupportElfFormat() const { return false; }

        // Choose to support parsing inlined asm instructions on specific platforms
        virtual bool SupportInlineAssembly() const { return false; }

        /// support predicate add pattern match
        virtual bool SupportMatchPredAdd() const { return false; }

        /// Adjust adapter to adjust the loop unrolling threshold
        virtual unsigned int GetLoopUnrollThreshold() const
        {
            return 4000;
        }

        // ----------------------------------------------------------------------
        // Below are workaround for bugs in front end or IGC will be removed once
        // the bugs are fixed

        /// Need workaround for A32 messages used along with A64
        virtual bool NeedWAToTransformA32MessagesToA64() const { return false; }

        /// disable mad in Vertex shader to avoid ZFigthing issues
        virtual bool DisabeMatchMad() const { return false; }

        /// WA bug in load store pattern match, needs to be cleaned up
        virtual bool WALoadStorePatternMatch() const { return false; }

        /// Some FE sends SLM pointers in DWORD units
        virtual bool WASLMPointersDwordUnit() const { return false; }

        /// Custom pass haven't been tested on all APIs
        virtual bool WADisableCustomPass() const { return false; }

        /// MemOpt2ForOCL pass not tested on all APIs
        virtual bool WAEnableMemOpt2ForOCL() const { return false; }

        /// disable some optimizations for front end which sends IR with unresolved NOS function when optimizing
        virtual bool WaNOSNotResolved() const { return false; }

        /// Temporarary disable GS attr const interpolation promotion
        virtual bool WADisableConstInterpolationPromotion() const { return false; }

        /// WA for APIs where frc generates a different precision than x - rndd(x) for small negative values
        /// Needs to switch to use fast math flags
        virtual bool DisableMatchFrcPatternMatch() const { return false; }

        /// Based on the type of inlined sampler we get we program different output.
        virtual bool ProgrammableBorderColorInCompute() const { return false; }

        /// WA for failures with HS with push constants
        virtual bool WaDisablePushConstantsForHS() const { return false; }

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

        /// Restrict dessa aliasing level. -1 : no restriction; max level otherwise.
        virtual int DessaAliasLevel() const { return -1; }

        /// Rounding mode used for DP emulated function, defaults to Round to nearest
        virtual unsigned DPEmulationRoundingMode() const { return 0; }

        /// Check for flushing denormals for DP emulated function
        virtual bool DPEmulationFlushDenorm() const { return false; }

        /// Check for flush to zero for DP emulated function
        virtual bool DPEmulationFlushToZero() const { return false; }

        // Maximum id that can be used by simple push constant buffers. The default is maximum unsigned int (no restriction)
        virtual unsigned int MaximumSimplePushBufferID() const { return std::numeric_limits<unsigned int>::max(); }

        /// Check if integer mad is enabled
        virtual bool EnableIntegerMad() const { return false; }

        /// add shader hash code after EOT for debug purposes
        virtual bool EnableShaderDebugHashCodeInKernel() const { return false; }


    };

}//namespace IGC
