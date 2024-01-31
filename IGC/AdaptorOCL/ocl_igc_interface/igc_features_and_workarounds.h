/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "cif/macros/enable.h"

// Interface : IGC_FE_WA
//             IGC features and workarounds
// Interface for defining target device features and workarounds

namespace IGC {

CIF_DECLARE_INTERFACE(IgcFeaturesAndWorkarounds, "IGC_FE_WA")

CIF_DEFINE_INTERFACE_VER(IgcFeaturesAndWorkarounds, 1){
  CIF_INHERIT_CONSTRUCTOR();

  virtual bool GetFtrDesktop() const;
  virtual void SetFtrDesktop(bool v);
  virtual bool GetFtrChannelSwizzlingXOREnabled() const;
  virtual void SetFtrChannelSwizzlingXOREnabled(bool v);

  virtual bool GetFtrGtBigDie() const;
  virtual void SetFtrGtBigDie(bool v);
  virtual bool GetFtrGtMediumDie() const;
  virtual void SetFtrGtMediumDie(bool v);
  virtual bool GetFtrGtSmallDie() const;
  virtual void SetFtrGtSmallDie(bool v);

  virtual bool GetFtrGT1() const;
  virtual void SetFtrGT1(bool v);
  virtual bool GetFtrGT1_5() const;
  virtual void SetFtrGT1_5(bool v);
  virtual bool GetFtrGT2() const;
  virtual void SetFtrGT2(bool v);
  virtual bool GetFtrGT3() const;
  virtual void SetFtrGT3(bool v);
  virtual bool GetFtrGT4() const;
  virtual void SetFtrGT4(bool v);

  virtual bool GetFtrIVBM0M1Platform() const;
  virtual void SetFtrIVBM0M1Platform(bool v);
  virtual bool GetFtrGTL() const;
  virtual void SetFtrGTL(bool v);
  virtual bool GetFtrGTM() const;
  virtual void SetFtrGTM(bool v);
  virtual bool GetFtrGTH() const;
  virtual void SetFtrGTH(bool v);
  virtual bool GetFtrSGTPVSKUStrapPresent() const;
  virtual void SetFtrSGTPVSKUStrapPresent(bool v);
  virtual bool GetFtrGTA() const;
  virtual void SetFtrGTA(bool v);
  virtual bool GetFtrGTC() const;
  virtual void SetFtrGTC(bool v);
  virtual bool GetFtrGTX() const;
  virtual void SetFtrGTX(bool v);
  virtual bool GetFtr5Slice() const;
  virtual void SetFtr5Slice(bool v);

  virtual bool GetFtrGpGpuMidThreadLevelPreempt() const;
  virtual void SetFtrGpGpuMidThreadLevelPreempt(bool v);
  virtual bool GetFtrIoMmuPageFaulting() const;
  virtual void SetFtrIoMmuPageFaulting(bool v);
  virtual bool GetFtrWddm2Svm() const;
  virtual void SetFtrWddm2Svm(bool v);
  virtual bool GetFtrPooledEuEnabled() const;
  virtual void SetFtrPooledEuEnabled(bool v);

  virtual bool GetFtrResourceStreamer() const;
  virtual void SetFtrResourceStreamer(bool v);

};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(IgcFeaturesAndWorkarounds, 2, 1) {
  CIF_INHERIT_CONSTRUCTOR();

  virtual void SetMaxOCLParamSize(uint32_t s);
  virtual uint32_t GetMaxOCLParamSize() const;
};


CIF_GENERATE_VERSIONS_LIST(IgcFeaturesAndWorkarounds);
CIF_MARK_LATEST_VERSION(IgcFeaturesAndWorkaroundsLatest, IgcFeaturesAndWorkarounds);

using IgcFeaturesAndWorkaroundsTagOCL = IgcFeaturesAndWorkarounds<2>; // transition time - remove this using
                                                                      // and uncomment the one below when finished

//using IgcFeaturesAndWorkaroundsTagOCL = IgcFeaturesAndWorkaroundsLatest; // Note : can tag with different version for
                                                                        //        transition periods
}

#include "cif/macros/disable.h"
