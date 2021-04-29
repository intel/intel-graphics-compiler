/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/platform.h"

#include "cif/export/pimpl_base.h"

#include "igfxfmid.h"

#include "cif/macros/enable.h"

namespace IGC {

CIF_DECLARE_INTERFACE_PIMPL(Platform) : CIF::PimplBase {
  CIF_PIMPL_DECLARE_CONSTRUCTOR()
  {
      memset(&p, 0, sizeof(p));
  }

  PLATFORM p;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(Platform);

}

#include "cif/macros/disable.h"
