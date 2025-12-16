/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "cif/macros/enable.h"

#include "OCLAPI/oclapi.h"

// Interface : IGC_FE_WA
//             IGC features and workarounds
// Interface for defining target device features and workarounds

namespace IGC {

CIF_DECLARE_INTERFACE(IgcFeaturesAndWorkarounds, "IGC_FE_WA")

CIF_DEFINE_INTERFACE_VER(IgcFeaturesAndWorkarounds, 1) {
  CIF_INHERIT_CONSTRUCTOR();

  OCL_API_CALL virtual bool GetFtrDesktop() const;
  OCL_API_CALL virtual void SetFtrDesktop(bool v);
  OCL_API_CALL virtual bool GetFtrChannelSwizzlingXOREnabled() const;
  OCL_API_CALL virtual void SetFtrChannelSwizzlingXOREnabled(bool v);

  OCL_API_CALL virtual bool GetFtrGtBigDie() const;
  OCL_API_CALL virtual void SetFtrGtBigDie(bool v);
  OCL_API_CALL virtual bool GetFtrGtMediumDie() const;
  OCL_API_CALL virtual void SetFtrGtMediumDie(bool v);
  OCL_API_CALL virtual bool GetFtrGtSmallDie() const;
  OCL_API_CALL virtual void SetFtrGtSmallDie(bool v);

  OCL_API_CALL virtual bool GetFtrGT1() const;
  OCL_API_CALL virtual void SetFtrGT1(bool v);
  OCL_API_CALL virtual bool GetFtrGT1_5() const;
  OCL_API_CALL virtual void SetFtrGT1_5(bool v);
  OCL_API_CALL virtual bool GetFtrGT2() const;
  OCL_API_CALL virtual void SetFtrGT2(bool v);
  OCL_API_CALL virtual bool GetFtrGT3() const;
  OCL_API_CALL virtual void SetFtrGT3(bool v);
  OCL_API_CALL virtual bool GetFtrGT4() const;
  OCL_API_CALL virtual void SetFtrGT4(bool v);

  OCL_API_CALL virtual bool GetFtrIVBM0M1Platform() const;
  OCL_API_CALL virtual void SetFtrIVBM0M1Platform(bool v);
  OCL_API_CALL virtual bool GetFtrGTL() const;
  OCL_API_CALL virtual void SetFtrGTL(bool v);
  OCL_API_CALL virtual bool GetFtrGTM() const;
  OCL_API_CALL virtual void SetFtrGTM(bool v);
  OCL_API_CALL virtual bool GetFtrGTH() const;
  OCL_API_CALL virtual void SetFtrGTH(bool v);
  OCL_API_CALL virtual bool GetFtrSGTPVSKUStrapPresent() const;
  OCL_API_CALL virtual void SetFtrSGTPVSKUStrapPresent(bool v);
  OCL_API_CALL virtual bool GetFtrGTA() const;
  OCL_API_CALL virtual void SetFtrGTA(bool v);
  OCL_API_CALL virtual bool GetFtrGTC() const;
  OCL_API_CALL virtual void SetFtrGTC(bool v);
  OCL_API_CALL virtual bool GetFtrGTX() const;
  OCL_API_CALL virtual void SetFtrGTX(bool v);
  OCL_API_CALL virtual bool GetFtr5Slice() const;
  OCL_API_CALL virtual void SetFtr5Slice(bool v);

  OCL_API_CALL virtual bool GetFtrGpGpuMidThreadLevelPreempt() const;
  OCL_API_CALL virtual void SetFtrGpGpuMidThreadLevelPreempt(bool v);
  OCL_API_CALL virtual bool GetFtrIoMmuPageFaulting() const;
  OCL_API_CALL virtual void SetFtrIoMmuPageFaulting(bool v);
  OCL_API_CALL virtual bool GetFtrWddm2Svm() const;
  OCL_API_CALL virtual void SetFtrWddm2Svm(bool v);
  OCL_API_CALL virtual bool GetFtrPooledEuEnabled() const;
  OCL_API_CALL virtual void SetFtrPooledEuEnabled(bool v);

  OCL_API_CALL virtual bool GetFtrResourceStreamer() const;
  OCL_API_CALL virtual void SetFtrResourceStreamer(bool v);
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(IgcFeaturesAndWorkarounds, 2, 1) {
  CIF_INHERIT_CONSTRUCTOR();

  OCL_API_CALL virtual void SetMaxOCLParamSize(uint32_t s);
  OCL_API_CALL virtual uint32_t GetMaxOCLParamSize() const;
};
CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(IgcFeaturesAndWorkarounds, 3, 2) { CIF_INHERIT_CONSTRUCTOR(); };

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(IgcFeaturesAndWorkarounds, 4, 2) {
  CIF_INHERIT_CONSTRUCTOR();

  virtual void SetFtrEfficient64BitAddressing(bool v);
  virtual bool GetFtrEfficient64BitAddressing() const;
};

CIF_GENERATE_VERSIONS_LIST(IgcFeaturesAndWorkarounds);
CIF_MARK_LATEST_VERSION(IgcFeaturesAndWorkaroundsLatest, IgcFeaturesAndWorkarounds);

using IgcFeaturesAndWorkaroundsTagOCL = IgcFeaturesAndWorkarounds<3>; // transition time - remove this using
                                                                      // and uncomment the one below when finished

// using IgcFeaturesAndWorkaroundsTagOCL = IgcFeaturesAndWorkaroundsLatest; // Note : can tag with different version for
//         transition periods
} // namespace IGC

#include "cif/macros/disable.h"
