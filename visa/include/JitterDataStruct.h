/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef JITTERDATASTRUCT_
#define JITTERDATASTRUCT_

#include <bitset>
#include <optional>
#include <stdint.h>

#include "llvm/Support/JSON.h"

namespace vISA {

struct VISA_BB_INFO {
  int id;
  unsigned staticCycle;
  unsigned sendStallCycle;
  unsigned char loopNestLevel;
};

struct FINALIZER_INFO {
  // ----- Required by IGC/VC/Runtime ----- //
  // Used by IGC for spill cost calculation
  bool isSpill = false;

  // spillMemUsed is the scratch size in byte of entire vISA stack for this
  // function/kernel. It contains spill size and caller/callee save size.
  unsigned int spillMemUsed = 0;

  // Debug info is callee allocated and populated only if switch is passed
  // to JIT to emit debug info.
  void *genDebugInfo = nullptr;
  unsigned int genDebugInfoSize = 0;

  // Propagate information about barriers presence back to IGC. It's safer to
  // depend on vISA statistics as IGC is not able to detect barriers if they
  // are used as a part of Inline vISA code.
  // This information is used by legacy CMRT as well as OpenCL/L0 runtime.
  unsigned numBarriers = 0;

  // Unweighted BB cycles counts. Used by IGC for SIMD width selection.
  unsigned BBNum = 0;
  VISA_BB_INFO *BBInfo = nullptr;

  // Whether kernel recompilation should be avoided. vISA hint for IGC.
  bool avoidRetry = false;

  // GTPin information
  void *freeGRFInfo = nullptr;
  unsigned int freeGRFInfoSize = 0;
  unsigned char numBytesScratchGtpin = 0;

  // Used by VC for setting execution environment in output
  bool hasStackcalls = false;

  // load-thread-payload prologs offset required by runtime
  // for skipping the prologs
  uint32_t offsetToSkipPerThreadDataLoad = 0;
  uint32_t offsetToSkipCrossThreadDataLoad = 0;

  // When two entries prolog is added for setting FFID
  // for compute (GP or GP1), skip this offset to set FFID_GP1.
  // Will set FFID_GP if not skip
  uint32_t offsetToSkipSetFFIDGP = 0;
  uint32_t offsetToSkipSetFFIDGP1 = 0;

  // ----- vISA Stats ----- //
  // Number of GRF acutally being used. Stats collection only.
  int numGRFUsed = 0;

  // Number of configured threads and GRF number. Used by IGC for
  // setting execution environment in output.
  uint32_t numGRFTotal = 0;
  uint32_t numThreads = 0;

  // Un-weighted asm instructions count. Used by IGC for spill
  // cost calculation
  int numAsmCount = 0;

  // Number of flag spill and fill. Used by VC Stats
  unsigned numFlagSpillStore = 0;
  unsigned numFlagSpillLoad = 0;

  // Number of spill/fill, weighted by loop. Used by IGC for
  // spill cost calculation.
  unsigned int numGRFSpillFill = 0;

  // ----- To be deprecated ----- //
  // These are vISA stats used only by IGC::CompilerStats
  // Subjet to deprecate
  uint32_t maxGRFPressure = 0;
  bool preRASchedulerForPressure = false;
  bool preRASchedulerForLatency = false;
  int64_t numCycles = 0;
  int64_t numSendInst = 0;
  int64_t numGRFSpill = 0;
  int64_t numGRFFill = 0;
  std::string raStatus;
};

} // namespace vISA
#endif // JITTERDATASTRUCT_
