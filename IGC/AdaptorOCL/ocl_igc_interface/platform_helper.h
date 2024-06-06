/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/cif.h"
#include "cif/common/id.h"

#include "ocl_igc_interface/platform.h"

#define COPY_VAL(INTEFACE_VAL_NAME, SRC_VAL_NAME)                              \
  dst.Set##INTEFACE_VAL_NAME(src.SRC_VAL_NAME)
#define COPY_VAL_E(VAL_NAME) dst.Set##VAL_NAME(src.e##VAL_NAME)
#define COPY_VAL_EXACT(VAL_NAME) dst.Set##VAL_NAME(src.VAL_NAME)

namespace IGC {
namespace PlatformHelper {
template <CIF::Version_t Ver, typename SrcStructT>
inline void PopulateInterfaceWith(IGC::Platform<Ver> &dst,
                                   const SrcStructT &src) {
  COPY_VAL_E(ProductFamily);
  COPY_VAL_E(PCHProductFamily);
  COPY_VAL_E(DisplayCoreFamily);
  COPY_VAL_E(RenderCoreFamily);
  COPY_VAL_E(PlatformType);
  COPY_VAL(DeviceID, usDeviceID);
  COPY_VAL(RevId, usRevId);
  COPY_VAL(DeviceID_PCH, usDeviceID_PCH);
  COPY_VAL(RevId_PCH, usRevId_PCH);
  COPY_VAL_E(GTType);
}

template <typename SrcStructT>
inline void PopulateInterfaceWith(IGC::Platform<2>& dst,
                                  const SrcStructT& src) {
  PopulateInterfaceWith<1>(dst, src);
  // Below COPY_VALs are not valid. NEO will populate *BlockID values
  // by calling the setters directly
  // COPY_VAL(RenderBlockID, sRenderBlockID.Value);
  // COPY_VAL(MediaBlockID, sMediaBlockID.Value);
  // COPY_VAL(DisplayBlockID, sDisplayBlockID.Value);
}
}

namespace GtSysInfoHelper {
template <CIF::Version_t Ver, typename SrcStructT>
inline void PopulateInterfaceWith(IGC::GTSystemInfo<Ver> &dst,
                                  const SrcStructT &src) {
  COPY_VAL_EXACT(EUCount);
  COPY_VAL_EXACT(ThreadCount);
  COPY_VAL_EXACT(SliceCount);
  COPY_VAL_EXACT(SubSliceCount);
  COPY_VAL_EXACT(L3CacheSizeInKb);
  COPY_VAL_EXACT(LLCCacheSizeInKb);
  COPY_VAL_EXACT(EdramSizeInKb);
  COPY_VAL_EXACT(L3BankCount);
  COPY_VAL_EXACT(MaxFillRate);
  COPY_VAL_EXACT(EuCountPerPoolMax);
  COPY_VAL_EXACT(EuCountPerPoolMin);

  COPY_VAL_EXACT(TotalVsThreads);
  COPY_VAL_EXACT(TotalHsThreads);
  COPY_VAL_EXACT(TotalDsThreads);
  COPY_VAL_EXACT(TotalGsThreads);
  COPY_VAL_EXACT(TotalPsThreadsWindowerRange);

  COPY_VAL_EXACT(CsrSizeInMb);

  COPY_VAL_EXACT(MaxEuPerSubSlice);
  COPY_VAL_EXACT(MaxSlicesSupported);
  COPY_VAL_EXACT(MaxSubSlicesSupported);

  COPY_VAL_EXACT(IsL3HashModeEnabled);

  COPY_VAL_EXACT(IsDynamicallyPopulated);
}

template <typename SrcStructT>
inline void PopulateInterfaceWith(IGC::GTSystemInfo<3>& dst,
                                  const SrcStructT& src) {
  PopulateInterfaceWith<1>(dst, src);
  COPY_VAL_EXACT(MaxDualSubSlicesSupported);
  COPY_VAL_EXACT(DualSubSliceCount);
}

}

namespace IgcPlatformFeaturesHelper {
template <CIF::Version_t Ver, typename SrcStructT>
inline void PopulateInterfaceWith(IGC::IgcFeaturesAndWorkarounds<Ver> &dst,
                                  const SrcStructT &src) {
  COPY_VAL_EXACT(FtrDesktop);
  COPY_VAL_EXACT(FtrChannelSwizzlingXOREnabled);

  COPY_VAL_EXACT(FtrGtBigDie);
  COPY_VAL_EXACT(FtrGtMediumDie);
  COPY_VAL_EXACT(FtrGtSmallDie);

  COPY_VAL_EXACT(FtrGT1);
  COPY_VAL_EXACT(FtrGT1_5);
  COPY_VAL_EXACT(FtrGT2);
  COPY_VAL_EXACT(FtrGT3);
  COPY_VAL_EXACT(FtrGT4);

  COPY_VAL_EXACT(FtrIVBM0M1Platform);
  COPY_VAL(FtrGTL, FtrGT1);
  COPY_VAL(FtrGTM, FtrGT2);
  COPY_VAL(FtrGTH, FtrGT3);
  COPY_VAL_EXACT(FtrSGTPVSKUStrapPresent);
  COPY_VAL_EXACT(FtrGTA);
  COPY_VAL_EXACT(FtrGTC);
  COPY_VAL_EXACT(FtrGTX);
  COPY_VAL_EXACT(Ftr5Slice);

  COPY_VAL_EXACT(FtrGpGpuMidThreadLevelPreempt);
  COPY_VAL_EXACT(FtrIoMmuPageFaulting);
  COPY_VAL_EXACT(FtrWddm2Svm);
  COPY_VAL_EXACT(FtrPooledEuEnabled);

  COPY_VAL_EXACT(FtrResourceStreamer);
}
}
}

#undef COPY_VAL_EXACT
#undef COPY_VAL_E
#undef COPY_VAL
