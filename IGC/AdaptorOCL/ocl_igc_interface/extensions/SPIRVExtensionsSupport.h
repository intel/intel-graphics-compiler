/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
// Public API for per-platform SPIR-V extension/capability queries.
//
// Thin by design: the impl (SPIRVExtensionsSupport.cpp) is the only TU that
// includes the generated .inc and spirv.hpp, so this header carries no SPIR-V
// header dependency and is safe to include anywhere.
//===----------------------------------------------------------------------===//

#ifndef IGCC_SPIRV_EXTENSIONS_SUPPORT_H
#define IGCC_SPIRV_EXTENSIONS_SUPPORT_H

#include <cstdint>
#include <vector>
#include <string>

#include "llvm/ADT/StringRef.h"
#include "igfxfmid.h"

namespace IGC {
namespace SPIRVExtensionsSupport {

// ===== Public types (consumed by YAML serialization and query callers) =====

struct SPIRVCapability {
  std::string Name;
  uint32_t Id;
};

struct SPIRVExtension {
  std::string Name;
  std::string SpecURL;
  std::vector<SPIRVCapability> Capabilities;
};

// ===== Public API =====

/// True if the named capability is supported on Platform. When
/// IncludeExperimental is set, experimental-tier support also counts.
bool isCapabilitySupported(llvm::StringRef CapabilityName, PLATFORM Platform, bool IncludeExperimental);

/// True if the named extension is supported on Platform. When
/// IncludeExperimental is set, experimental-tier support also counts.
bool isExtensionSupported(llvm::StringRef ExtensionName, PLATFORM Platform, bool IncludeExperimental);

/// Returns all extensions with their capabilities (metadata only).
const std::vector<SPIRVExtension> &getAllExtensions();

/// Returns the extensions (and their capabilities) supported on Platform.
std::vector<SPIRVExtension> getSupportedExtensionInfo(PLATFORM Platform, bool IncludeExperimental = false);

} // namespace SPIRVExtensionsSupport
} // namespace IGC

#endif // IGCC_SPIRV_EXTENSIONS_SUPPORT_H
