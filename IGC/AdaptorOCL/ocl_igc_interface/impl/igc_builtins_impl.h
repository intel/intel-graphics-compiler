/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/igc_builtins.h"
#include "cif/export/pimpl_base.h"
#include "cif/macros/enable.h"

namespace IGC {

CIF_DECLARE_INTERFACE_PIMPL(IgcBuiltins) : CIF::PimplBase {

  CIF_PIMPL_DECLARE_CONSTRUCTOR()
  {
  }

};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(IgcBuiltins);

}

#include "cif/macros/disable.h"
