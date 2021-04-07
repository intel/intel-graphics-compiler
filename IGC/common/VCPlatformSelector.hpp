/*========================== begin_copyright_notice ============================

Copyright (c) 2020-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

// VC platform selection process need to be used in two places:
// * igcdeps library to determine platform for SPIRV compilation
// * fcl library to determine platform for sources compilation
// this is header-only because we do not want link dependencies
// from library with only two functions

#ifndef VC_PLATFORM_SELECTOR_H
#define VC_PLATFORM_SELECTOR_H

#include "Probe/Assertion.h"
#include "StringMacros.hpp"
#include "igfxfmid.h"

namespace cmc {

inline const char *getPlatformStr(PLATFORM Platform, unsigned &RevId) {
  // after some consultations with Wndows KMD folks,
  // only render core shall be used in all cases
  auto Core = Platform.eRenderCoreFamily;
  auto Product = Platform.eProductFamily;
  IGC_ASSERT(RevId == Platform.usRevId);

  switch (Core) {
  case IGFX_GEN9_CORE:
    return "SKL";
  case IGFX_GEN10_CORE:
    return "CNL";
  case IGFX_GEN11_CORE:
    if (Product == IGFX_ICELAKE_LP || Product == IGFX_LAKEFIELD)
      return "ICLLP";
    return "ICL";
  case IGFX_GEN12_CORE:
  case IGFX_GEN12LP_CORE:
    if (Product == IGFX_TIGERLAKE_LP)
      return "TGLLP";
  default:
    IGC_ASSERT_MESSAGE(0, "unsupported platform");
    break;
  }
  return IGC_MANGLE("SKL");
}

} // namespace cmc

#endif
