/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/igc_regkeys.hpp"

// In _RELEASE builds, make these api functions available for internal use,
// but do not export them in the dll.
#if defined(IGC_DEBUG_VARIABLES)
#if defined(_WIN32)
#if defined(IGC_EXPORTS)
#define IGC_DEBUG_API_CALL __declspec(dllexport)
#else
#define IGC_DEBUG_API_CALL __declspec(dllimport)
#endif
#else
#if defined(IGC_EXPORTS)
#define IGC_DEBUG_API_CALL __attribute__((visibility("default")))
#else
#define IGC_DEBUG_API_CALL
#endif
#endif
#else
#define IGC_DEBUG_API_CALL
#endif

namespace IGC {
namespace Debug {

using OutputFolderName = const char *;

#if defined(IGC_DEBUG_VARIABLES)

OutputFolderName GetBaseIGCOutputFolder();
OutputFolderName GetShaderOutputFolder();
void IGC_DEBUG_API_CALL SetShaderOutputFolder(OutputFolderName name);

#else // !IGC_DEBUG_VARIABLES

inline OutputFolderName GetBaseIGCOutputFolder() { return ""; }
inline OutputFolderName GetShaderOutputFolder() { return ""; }
inline IGC_DEBUG_API_CALL void SetShaderOutputFolder(OutputFolderName name) { (void)name; }

#endif // IGC_DEBUG_VARIABLES

} // namespace Debug
} // namespace IGC
