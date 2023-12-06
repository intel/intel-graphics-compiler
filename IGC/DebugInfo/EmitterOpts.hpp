/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef EMITTEROPTIONS_HPP_0NAXOILP
#define EMITTEROPTIONS_HPP_0NAXOILP

namespace IGC {
struct DebugEmitterOpts {
  bool DebugEnabled = false;
  bool EnableRelocation = false;
  bool ZeBinCompatible = false;
  bool EnforceAMD64Machine = false;
  bool ScratchOffsetInOW = true;
  bool EmitATLinkageName = true;
  bool EnableDebugInfoValidation = false;
  unsigned int VISAABIVersion = 2;
};
} // namespace IGC

#endif /* end of include guard: EMITTEROPTIONS_HPP_0NAXOILP */
