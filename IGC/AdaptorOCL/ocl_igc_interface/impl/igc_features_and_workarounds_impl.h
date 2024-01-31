/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/igc_features_and_workarounds.h"

#include "cif/export/pimpl_base.h"
#include "cif/helpers/memory.h"

#include "Compiler/compiler_caps.h"
#include "usc.h"

#include "cif/macros/enable.h"


namespace IGC {

CIF_DECLARE_INTERFACE_PIMPL(IgcFeaturesAndWorkarounds) : CIF::PimplBase {

  CIF_PIMPL_DECLARE_CONSTRUCTOR() {
      CIF::SafeZeroOut(FeTable);
  }

  _SUscSkuFeatureTable FeTable;
  OCLCaps OCLCaps;

  // VISA-WA - OPEN : Maybe IGC would prefer to get WA table instead of generating one on its own?
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(IgcFeaturesAndWorkarounds);

}

#include "cif/macros/disable.h"
