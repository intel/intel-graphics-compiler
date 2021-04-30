/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef EMITTEROPTIONS_HPP_0NAXOILP
#define EMITTEROPTIONS_HPP_0NAXOILP

namespace IGC
{
  struct DebugEmitterOpts {
    bool DebugEnabled = false;
    bool isDirectElf = false;
    bool UseNewRegisterEncoding = true;
    bool EnableSIMDLaneDebugging = true;
    bool EnableGTLocationDebugging = false;
    bool UseOffsetInLocation = false;
    bool EmitDebugRanges = false;
    bool EmitDebugLoc = false;
    bool EmitOffsetInDbgLoc = false;
    bool EnableRelocation = false;
    bool EnableElf2ZEBinary = false;
    bool EmitPrologueEnd = true;
    bool ScratchOffsetInOW = true;
    bool EmitATLinkageName = true;
  };
}

#endif /* end of include guard: EMITTEROPTIONS_HPP_0NAXOILP */

