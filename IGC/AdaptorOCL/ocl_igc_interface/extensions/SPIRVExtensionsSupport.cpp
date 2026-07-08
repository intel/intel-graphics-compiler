/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
// Implementation of SPIRVExtensionsSupport.h.
//
// The only TU that includes the generated .inc and spirv.hpp. The .inc names
// spv::Capability* enums for non-experimental extensions, so this file is built
// with the OCL SPIRV-Headers first on its include path (per-source property in
// Source/IGC/CMakeLists.txt).
//
// Each capability/extension carries two predicates (production, experimental) of
// type bool(*)(PLATFORM); nullptr means "unsupported in that tier". Pred::
// mirrors the .td vocabulary so the generated .inc reads like the .td.
//===----------------------------------------------------------------------===//

#include "SPIRVExtensionsSupport.h"

#include "spirv/unified1/spirv.hpp"

namespace IGC {
namespace SPIRVExtensionsSupport {

// ===== Platform predicates =====

using PlatformPred = bool (*)(PLATFORM);

namespace Pred {

constexpr bool AllPlatformSupport(PLATFORM) { return true; }
constexpr bool NotSupported(PLATFORM) { return false; }

template <GFXCORE_FAMILY MinCore> constexpr bool isCoreChildOf(PLATFORM P) { return P.eRenderCoreFamily >= MinCore; }

template <GFXCORE_FAMILY Core> constexpr bool ExactCoreFamily(PLATFORM P) { return P.eRenderCoreFamily == Core; }

template <PRODUCT_FAMILY MinProd> constexpr bool isProductChildOf(PLATFORM P) { return P.eProductFamily >= MinProd; }

template <PRODUCT_FAMILY Prod> constexpr bool ExactPlatform(PLATFORM P) { return P.eProductFamily == Prod; }

template <PRODUCT_FAMILY... Ps> constexpr bool isInGroup(PLATFORM P) { return ((P.eProductFamily == Ps) || ...); }

template <PlatformPred... Cs> constexpr bool AllOf(PLATFORM P) { return (... && Cs(P)); }

template <PlatformPred... Cs> constexpr bool AnyOf(PLATFORM P) { return (... || Cs(P)); }

template <PlatformPred Base> constexpr bool Not(PLATFORM P) { return !Base(P); }

} // namespace Pred

// ===== Internal data =====

namespace detail {

struct CapabilityDef {
  const char *Name;
  PlatformPred Production;   // nullptr = no production-tier support
  PlatformPred Experimental; // nullptr = no experimental-tier support
  uint32_t Id;               // SPIR-V numeric capability ID (0 = not reported)
};

struct ExtensionDef {
  const char *Name;
  const char *URL;
  // Both nullptr = InheritFromCapabilities (aggregate from capabilities).
  // Otherwise, each tier predicate is independent: nullptr = no support in
  // that tier, non-null = the platform rule for that tier.
  PlatformPred Production;
  PlatformPred Experimental;
  std::vector<CapabilityDef> Capabilities;
};

static const std::vector<ExtensionDef> &getExtensions() {
  static const std::vector<ExtensionDef> Extensions = {
#include "SPIRVExtensionsSupport.inc"
  };
  return Extensions;
}

// True if the platform satisfies either tier per the gating rule.
static bool tierMatch(PlatformPred Prod, PlatformPred Exp, PLATFORM P, bool IncludeExp) {
  if (Prod && Prod(P))
    return true;
  return IncludeExp && Exp && Exp(P);
}

} // namespace detail

// ===== Public API =====

bool isCapabilitySupported(llvm::StringRef CapabilityName, PLATFORM Platform, bool IncludeExperimental) {
  for (const auto &Ext : detail::getExtensions())
    for (const auto &Cap : Ext.Capabilities)
      if (CapabilityName == Cap.Name)
        return detail::tierMatch(Cap.Production, Cap.Experimental, Platform, IncludeExperimental);
  return false;
}

bool isExtensionSupported(llvm::StringRef ExtensionName, PLATFORM Platform, bool IncludeExperimental) {
  for (const auto &Ext : detail::getExtensions()) {
    if (ExtensionName != Ext.Name)
      continue;
    if (Ext.Production || Ext.Experimental)
      return detail::tierMatch(Ext.Production, Ext.Experimental, Platform, IncludeExperimental);
    // Both nullptr = InheritFromCapabilities: supported if any capability is.
    for (const auto &Cap : Ext.Capabilities)
      if (detail::tierMatch(Cap.Production, Cap.Experimental, Platform, IncludeExperimental))
        return true;
    return false;
  }
  return false;
}

const std::vector<SPIRVExtension> &getAllExtensions() {
  static const std::vector<SPIRVExtension> All = [] {
    std::vector<SPIRVExtension> Result;
    for (const auto &E : detail::getExtensions()) {
      SPIRVExtension Ext;
      Ext.Name = E.Name;
      Ext.SpecURL = E.URL;
      for (const auto &Cap : E.Capabilities)
        Ext.Capabilities.push_back({Cap.Name, Cap.Id});
      Result.push_back(std::move(Ext));
    }
    return Result;
  }();
  return All;
}

std::vector<SPIRVExtension> getSupportedExtensionInfo(PLATFORM Platform, bool IncludeExperimental) {
  std::vector<SPIRVExtension> SupportedExtensions;
  for (const auto &Ext : getAllExtensions()) {
    if (!isExtensionSupported(Ext.Name, Platform, IncludeExperimental))
      continue;
    SPIRVExtension SupportedExt;
    SupportedExt.Name = Ext.Name;
    SupportedExt.SpecURL = Ext.SpecURL;
    for (const auto &Cap : Ext.Capabilities)
      if (isCapabilitySupported(Cap.Name, Platform, IncludeExperimental))
        SupportedExt.Capabilities.push_back(Cap);
    SupportedExtensions.push_back(std::move(SupportedExt));
  }
  return SupportedExtensions;
}

} // namespace SPIRVExtensionsSupport
} // namespace IGC
