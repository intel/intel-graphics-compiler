/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

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
