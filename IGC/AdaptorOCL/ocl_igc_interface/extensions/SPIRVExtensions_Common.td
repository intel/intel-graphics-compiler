/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef SPIRV_EXTENSIONS_COMMON_TD
#define SPIRV_EXTENSIONS_COMMON_TD

// This TableGen file supports flexible platform specification for SPIR-V extensions
// and capabilities. Here are the supported use cases:
//
// 1. BASIC PLATFORM SUPPORT TYPES:
//    - AllPlatformSupport: Supported on all platforms
//    - isCoreChildOf<CoreFamily>: Supported on specified core family and newer
//    - ExactPlatform<Platform>: Supported only on the exact platform specified
//    - isInGroup<PlatformGroup>: Supported on any platform in the group
//    - AnyOf<[condition1, condition2, ...]>: Supported if ANY condition matches (OR logic)
//    - NotSupported: Capability/extension is not supported on any platform
//    - InheritFromExtension: Implicit capability default (omit Support to inherit from extension; do NOT specify explicitly)
//    - InheritFromCapabilities: Extension-level aggregation mode; extension platform support becomes the
//        aggregate of all capability supports. All capabilities must declare explicit platform
//        support (cannot use InheritFromExtension). Capabilities marked NotSupported are ignored
//        when computing the aggregate.
//
// 2. PLATFORM SUPPORT MODES & RULES:
//
// Two mutually exclusive modes define how platform support is specified and validated.
//
//    MODE 1: Extension-Level Platform Support ("Regular" Extensions)
//      Summary:
//        * Extension declares a single platform support descriptor (AllPlatformSupport, isCoreChildOf, ExactPlatform, isInGroup, AnyOf, etc.).
//        * Capabilities DO NOT declare platform support; by omitting it they implicitly use InheritFromExtension.
//      Constraints / Validation:
//        * Extension MUST have explicit platform support (cannot use InheritFromExtension itself).
//        * If any capability declares explicit platform support (i.e. not InheritFromExtension) → build error.
//      Effect:
//        * All capabilities share identical platform support; guarantees consistency.
//      Example:
//        def RegularExtensionExample : Extension<
//          "RegularExtensionExample",
//          [ Capability<"CapA">,
//            Capability<"CapB"> ],
//          "https://example.com/spec/RegularExtensionExample",
//          isCoreChildOf<CORE_FAMILY>
//        >; // CapA/CapB inherit extension support (implicit InheritFromExtension).
//        // Adding explicit platform support to CapA/CapB would be a validation error.
//
//    MODE 2: Capability-Level Platform Support (InheritFromCapabilities Extensions)
//      Summary:
//        * Extension uses InheritFromCapabilities as its ExtSupport.
//        * Each capability MUST declare its own explicit platform support descriptor (cannot use InheritFromExtension).
//      Constraints / Validation:
//        * Any capability using InheritFromExtension → build error.
//        * Capabilities marked NotSupported are excluded from aggregation.
//      Effect:
//        * Extension platform support becomes the aggregate of all capability supports (logical OR of supported platforms).
//      Example:
//        def AggregateExtensionExample : Extension<
//          "AggregateExtensionExample",
//          [ Capability<"CapC", isCoreChildOf<CORE_FAMILY>>,
//            Capability<"CapD", ExactPlatform<PLATFORM>>,
//            Capability<"CapE", AllPlatformSupport>,
//            Capability<"CapF", NotSupported> ],
//          "https://example.com/spec/AggregateExtensionExample",
//          InheritFromCapabilities
//        >; // Effective support is OR of CapC/CapD/CapE (CapF ignored).
//
// 3. RATIONALE:
//
//    This design eliminates ambiguity and ensures consistency:
//    - If you want uniform platform support → use regular extension with explicit platform
//    - If you want per-capability platform support → use InheritFromCapabilities
//    - No mixing of the two approaches in a single extension

class Platform<string ProductFamilyEnum> {
  string ProductFamily = ProductFamilyEnum;
}

class CoreFamily<string CoreFamilyEnum> {
  string RenderCoreFamily = CoreFamilyEnum;
}

class PlatformGroup<list<Platform> GroupPlatforms> {
  list<Platform> Platforms = GroupPlatforms;
}

// Platform support specification classes
class PlatformSupport;

class isCoreChildOf<CoreFamily baseCore> : PlatformSupport {
  CoreFamily BaseCore = baseCore;
}

class ExactPlatform<Platform platform> : PlatformSupport {
  Platform TargetPlatform = platform;
}

class isInGroup<PlatformGroup group> : PlatformSupport {
  PlatformGroup TargetGroup = group;
}

class AnyOf<list<PlatformSupport> conditions> : PlatformSupport {
  list<PlatformSupport> Conditions = conditions;
}

// Special cases
def AllPlatformSupport : PlatformSupport;
def InheritFromCapabilities : PlatformSupport; // Aggregated capability-mode
def NotSupported : PlatformSupport;
def InheritFromExtension : PlatformSupport;  // Default for capabilities - inherit from extension

// Base SPIRV classes
class SPIRVCapability<string name, PlatformSupport support = InheritFromExtension> {
  string Name = name;
  PlatformSupport Support = support;
}

class SPIRVExtension<string name, list<SPIRVCapability> caps, string url, PlatformSupport support = AllPlatformSupport, bit published = 1> {
  string ExtName = name;
  list<SPIRVCapability> ExtCapabilities = caps;
  string ExtSpecURL = url;
  PlatformSupport ExtSupport = support;
  bit Published = published;
}

// Convenience aliases for cleaner syntax
class Extension<string name, list<SPIRVCapability> caps, string url, PlatformSupport support = AllPlatformSupport, bit published = 1>
  : SPIRVExtension<name, caps, url, support, published>;

class Capability<string name, PlatformSupport support = InheritFromExtension>  // Default: inherit from extension
  : SPIRVCapability<name, support>;

#endif // SPIRV_EXTENSIONS_COMMON_TD
