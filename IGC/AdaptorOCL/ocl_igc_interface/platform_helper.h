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
inline void PopulateInterfaceWith(IGC::GTSystemInfo<2>& dst,
                                  const SrcStructT& src) {
  PopulateInterfaceWith<1>(dst, src);
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
