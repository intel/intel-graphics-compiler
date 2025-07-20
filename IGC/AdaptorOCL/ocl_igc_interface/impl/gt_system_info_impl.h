/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/gt_system_info.h"

#include "cif/export/pimpl_base.h"
#include "cif/helpers/memory.h"

#include "gtsysinfo.h"

#include "cif/macros/enable.h"
#include "OCLAPI/oclapi.h"

namespace IGC {

CIF_DECLARE_INTERFACE_PIMPL(GTSystemInfo) : CIF::PimplBase {
  OCL_API_CALL CIF_PIMPL_DECLARE_CONSTRUCTOR() { CIF::SafeZeroOut(gsi); }

  GT_SYSTEM_INFO gsi;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(GTSystemInfo);

} // namespace IGC

#include "cif/macros/disable.h"
