/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/igc_options_and_capabilities.h"
#include "ocl_igc_interface/impl/platform_impl.h"

#include "cif/export/pimpl_base.h"
#include "cif/helpers/error.h"
#include "cif/helpers/memory.h"

#include "cif/export/muiltiversion.h"
#include "cif/export/pimpl_base.h"

#include "cif/macros/enable.h"
#include "OCLAPI/oclapi.h"

namespace IGC {

CIF_DECLARE_INTERFACE_PIMPL(IgcOptionsAndCapabilities) : CIF::PimplBase {
  CIF_PIMPL_DECLARE_CONSTRUCTOR() { this->platformPtr = nullptr; }

  void SetPlatform(CIF::Multiversion<Platform> * platformPtr) { this->platformPtr = platformPtr; }

  CIF::Multiversion<Platform> *GetPlatform() const { return this->platformPtr; }

protected:
  CIF::Multiversion<Platform> *platformPtr;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(IgcOptionsAndCapabilities);
} // namespace IGC

#include "cif/macros/disable.h"
