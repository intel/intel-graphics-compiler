#ifndef EMITTEROPTIONS_HPP_0NAXOILP
#define EMITTEROPTIONS_HPP_0NAXOILP

namespace IGC
{
  struct DebugEmitterOpts {
    bool isDirectElf = false;
    bool UseNewRegisterEncoding = true;
    bool EnableSIMDLaneDebugging = true;
    bool EnableGTLocationDebugging = false;
    bool EmitDebugRanges = false;
    bool EmitDebugLoc = false;
    bool EmitOffsetInDbgLoc = false;
  };
}

#endif /* end of include guard: EMITTEROPTIONS_HPP_0NAXOILP */

