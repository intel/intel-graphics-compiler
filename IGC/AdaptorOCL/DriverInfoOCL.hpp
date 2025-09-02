/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/DriverInfo.hpp"

namespace TC {
/// caps common to all OCL runtimes
class CDriverInfoOCLCommon : public IGC::CDriverInfo {
public:
  bool AllowUnsafeHalf() const override { return false; }

  bool AllowSendFusion() const override { return false; }

  bool SupportFastestStage1() const override { return true; }

  bool SupportsIEEEMinMax() const override { return true; }

  bool NeedCheckContractionAllowed() const override { return true; }

  bool NeedI64BitDivRem() const override { return true; }

  bool HasMemoryIntrinsics() const override { return true; }

  bool HasNonNativeLoadStore() const override { return true; }

  bool NeedLoweringInlinedConstants() const override { return true; }
  bool benefitFromTypeDemotion() const override { return true; }
  bool benefitFromPreRARematFlag() const override { return true; }

  bool NeedExtraPassesAfterAlwaysInlinerPass() const override { return true; }
  bool enableVISAPreRAScheduler() const override { return true; }
  bool enableVISAPreRASchedulerForRetry() const override { return true; }

  bool NeedWAToTransformA32MessagesToA64() const override { return true; }
  bool WADisableCustomPass() const override { return false; }
  bool WAEnableMemOpt2ForOCL() const override { return true; }

  unsigned int GetLoopUnrollThreshold() const override { return 1280; }
  bool Enable64BitEmu() const override { return true; }

  bool NeedIEEESPDiv() const override { return true; }

  // Not needed as OCL doesn't go through emitStore3DInner
  bool splitUnalignedVectors() const override { return false; }

  bool supportsStatelessSpacePrivateMemory() const override { return true; }

  bool NeedFP64(PRODUCT_FAMILY productFamily) const override { return IGC_IS_FLAG_ENABLED(EnableDPEmulation); }

  bool NeedFP64DivSqrt() const override { return true; }

  bool NeedFP64toFP16Conv() const override { return true; }

  bool EnableIntegerMad() const override { return true; }

  bool RespectPerInstructionContractFlag() const override { return true; }

  bool EnableLSCForLdRawAndStoreRawOnDG2() const override { return true; }

  bool supportLscSamplerRouting() const override { return false; }

  bool EnableShaderDebugHashCodeInKernel() const override { return true; }

};

// In case some cpas are specific to NEO
class CDriverInfoOCLNEO : public CDriverInfoOCLCommon {
public:
  bool SupportsStatelessToStatefulBufferTransformation() const override { return true; }
  unsigned getVISAPreRASchedulerCtrl() const override { return 6; }
  bool SupportStatefulToken() const override { return true; }
  bool SupportInlineAssembly() const override { return true; }
  /// Enables the use of inline data on XeHP_SDV+
  virtual bool UseInlineData() const override { return true; }
  bool getLscStoresWithNonDefaultL1CacheControls() const override { return false; }
};

} // namespace TC
