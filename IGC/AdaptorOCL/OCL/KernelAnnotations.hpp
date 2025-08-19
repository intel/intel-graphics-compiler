/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _KERNEL_ANNOTATIONS_H_
#define _KERNEL_ANNOTATIONS_H_

#include <string>
#include <vector>

#include "OCL/sp/sp_types.h"

namespace iOpenCL {

struct InitConstantAnnotation {
  std::vector<unsigned char> InlineData;
  int Alignment;
  size_t AllocSize;
};

struct InitGlobalAnnotation {
  std::vector<unsigned char> InlineData;
  int Alignment;
  size_t AllocSize;
};

struct ThreadPayload {
  bool HasLocalIDx = false;
  bool HasLocalIDy = false;
  bool HasLocalIDz = false;
  bool HasGlobalIDOffset = false;
  bool HasGroupID = false;
  bool HasLocalID = false;
  bool HasFlattenedLocalID = false;
  bool CompiledForIndirectPayloadStorage = false;
  bool UnusedPerThreadConstantPresent = false;
  bool HasStageInGridOrigin = false;
  bool HasStageInGridSize = false;
  uint32_t PassInlineDataSize = 0;
  uint32_t OffsetToSkipPerThreadDataLoad = 0;
  uint32_t OffsetToSkipSetFFIDGP = 0;
  bool HasRTStackID = false;
  bool generateLocalID = false;
  uint32_t emitLocalMask = 0;
  uint32_t walkOrder = 0;
  bool tileY = false;
};

struct ExecutionEnvironment {
  DWORD CompiledSIMDSize = 0;
  DWORD CompiledSubGroupsNumber = 0;
  // legacy design:hold all ScratchSpaceUsage
  // new design:   hold spillfill+callstack+GTPin
  // Todo: rename it to m_PerThreadScratchSpaceSlot0
  DWORD PerThreadScratchSpace = 0;
  // DWORD  PerThreadScratchUseGtpin                   = 0;
  // legacy design:not used
  // new design:   hold private memory used by shader if non-ZERO
  DWORD PerThreadScratchSpaceSlot1 = 0;
  // Size in bytes of the stateless memory requirement for allocating
  // private variables.
  DWORD PerThreadPrivateOnStatelessSize = 0;
  // legacy design:not used
  // new design:   hold private memory used by shader if non-ZERO
  DWORD SumFixedTGSMSizes = 0;
  bool HasDeviceEnqueue = false;
  // for PVC+ targets this field preserves the number of barriers
  uint32_t HasBarriers = 0;
  bool HasSample = false;
  bool IsSingleProgramFlow = false;
  // DWORD  PerSIMDLanePrivateMemorySize               = 0;
  bool HasFixedWorkGroupSize = false;
  bool HasReadWriteImages = false;
  bool DisableMidThreadPreemption = false;
  bool IsInitializer = false;
  bool IsFinalizer = false;
  bool SubgroupIndependentForwardProgressRequired = false;
  bool CompiledForGreaterThan4GBBuffers = false;
  DWORD FixedWorkgroupSize[3] = {};
  DWORD NumGRFRequired = 0;
  DWORD WorkgroupWalkOrder[3] = {};
  bool HasGlobalAtomics = false;
  bool UseBindlessMode = false;
  bool HasDPAS = false;
  bool HasRTCalls = false;
  DWORD StatelessWritesCount = 0;
  DWORD IndirectStatelessCount = 0;
  DWORD numThreads = 0;
  bool HasPrintfCalls = false;
  bool HasIndirectCalls = false;
  bool HasStackCalls = false;
  bool RequireDisableEUFusion = false;
  DWORD PerThreadSpillMemoryUsage = 0;
  DWORD PerThreadPrivateMemoryUsage = 0;
  bool HasLscStoresWithNonDefaultL1CacheControls = false;
};

} // namespace iOpenCL

#endif
