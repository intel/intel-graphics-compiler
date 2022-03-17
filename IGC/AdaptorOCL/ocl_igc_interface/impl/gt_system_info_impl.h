/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/gt_system_info.h"

#include "cif/export/pimpl_base.h"
#include "cif/helpers/memory.h"

#include "ocl_igc_interface/impl/gt_slice_info_impl.h"

#include "gtsysinfo.h"

#include "cif/macros/enable.h"

namespace IGC {

CIF_DECLARE_INTERFACE_PIMPL(GTSystemInfo) : CIF::PimplBase {
  CIF_PIMPL_DECLARE_CONSTRUCTOR() {
      CIF::SafeZeroOut(gsi);
  }

  void SetSliceCount(uint32_t sliceCount){
      gsi.SliceCount = sliceCount;
      if(sliceInfo.size() > sliceCount){
          sliceInfo.resize(sliceCount);
      }else {
          sliceInfo.reserve(sliceCount);
          while(sliceInfo.size() > sliceCount){
              sliceInfo.push_back(std::make_unique<CIF::Multiversion<GTSliceInfo>>());
              (*sliceInfo.rbegin())->CreateImpl();
          }
      }
  }

  GTSliceInfoBase *GetSliceInfoHandle(CIF::Version_t version, uint32_t sliceIdx) {
      return sliceInfo.at(sliceIdx)->GetVersion(version);
  }

  GT_SYSTEM_INFO gsi;
  std::vector<std::unique_ptr<CIF::Multiversion<GTSliceInfo>>> sliceInfo;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(GTSystemInfo);

}

#include "cif/macros/disable.h"
