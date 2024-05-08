/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/ocl_gen_binary.h"

#include "cif/export/pimpl_base.h"

#include "igfxfmid.h"

#include "cif/macros/enable.h"

namespace IGC {

CIF_DECLARE_INTERFACE_PIMPL(OclGenBinary) : CIF::PimplBase {
  CIF_PIMPL_DECLARE_CONSTRUCTOR()
  {
  }
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(OclGenBinary);

}

#include "cif/macros/disable.h"
