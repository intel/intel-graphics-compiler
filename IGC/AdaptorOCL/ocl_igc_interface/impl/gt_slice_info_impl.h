/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/gt_slice_info.h"
#include "ocl_igc_interface/gt_dss_info.h"

#include "cif/export/pimpl_base.h"
#include "cif/export/muiltiversion.h"
#include "cif/helpers/memory.h"

#include "ocl_igc_interface/impl/gt_dss_info_impl.h"

#include "cif/macros/enable.h"

#include <vector>

namespace IGC {

CIF_DECLARE_INTERFACE_PIMPL(GTSliceInfo) : CIF::PimplBase {
  CIF_PIMPL_DECLARE_CONSTRUCTOR() = default;

  void SetDualSubsliceCount(uint32_t dssCount){
      if(dssInfo.size() > dssCount){
          dssInfo.resize(dssCount);
      }else {
          dssInfo.reserve(dssCount);
          while(dssInfo.size() > dssCount){
              dssInfo.push_back(std::make_unique<CIF::Multiversion<GTDualSubSliceInfo>>());
              (*dssInfo.rbegin())->CreateImpl();
          }
      }
  }

  GTDualSubSliceInfoBase *GetDualSubSliceInfoHandle(CIF::Version_t version, uint32_t dssIdx) {
      return dssInfo.at(dssIdx)->GetVersion(version);
  }

  uint32_t GetDualSubSliceCount() const {
      return static_cast<uint32_t>(dssInfo.size());
  }

  bool enabled = false;
  std::vector<std::unique_ptr<CIF::Multiversion<GTDualSubSliceInfo>>> dssInfo;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(GTSliceInfo);

}

#include "cif/macros/disable.h"
