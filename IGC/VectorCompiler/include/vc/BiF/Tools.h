/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_BIF_TOOLS_H
#define VC_BIF_TOOLS_H

namespace vc {
namespace bif {

// Whether VC BiF generation is disabled.
inline constexpr bool disabled() {
#ifdef IGC_VC_DISABLE_BIF
  return true;
#else  // IGC_VC_DISABLE_BIF
  return false;
#endif // IGC_VC_DISABLE_BIF
}

} // namespace bif
} // namespace vc

#endif
