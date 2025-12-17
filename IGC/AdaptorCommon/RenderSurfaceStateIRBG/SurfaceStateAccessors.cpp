/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This file is processed by the IRBuilderGenerator tool.
///
//===----------------------------------------------------------------------===//

// Set this so that __builtin_offset() is used so the result of offsetof() is
// a constexpr on windows.
#ifdef _WIN32
#define _CRT_USE_BUILTIN_OFFSETOF 1
#endif // _WIN32

#include "SurfaceStateFormat.h"
#include "BuilderUtils.h"
#include "AutoGenAddressSpaces.h"

using namespace IGC;

CREATE_PRIVATE auto _loadSurfaceBaseAddress_64B(SurfaceStateHeapAS void *heapBasePtr, uint32_t surfaceStateOffset) {
  SurfaceStateHeapAS RENDER_SURFACE_STATE *surfaceStatePtr =
      (SurfaceStateHeapAS RENDER_SURFACE_STATE *)((uint64_t)heapBasePtr + surfaceStateOffset);
  return surfaceStatePtr->SurfaceBaseAddress;
}
