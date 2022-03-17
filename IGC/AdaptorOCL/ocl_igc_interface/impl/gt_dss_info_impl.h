/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/gt_dss_info.h"

#include "cif/export/pimpl_base.h"
#include "cif/helpers/memory.h"

#include "cif/macros/enable.h"

namespace IGC {

CIF_DECLARE_INTERFACE_PIMPL(GTDualSubSliceInfo) : CIF::PimplBase {
  CIF_PIMPL_DECLARE_CONSTRUCTOR() = default;

  bool enabled = false;
  uint32_t euEnabledCount = 0U;
  uint32_t euEnabledMask = 0U;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(GTDualSubSliceInfo);

}

#include "cif/macros/disable.h"
