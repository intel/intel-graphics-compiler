/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*==============================================================================
 * Platform support types and helpers for IGCCSPIRVSupportTblGen
 *============================================================================*/
#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/Error.h"
#include "llvm/Support/SourceMgr.h"

extern llvm::SourceMgr llvm::SrcMgr;

namespace IGCCSPIRVPlatformSupport {
// Selective LLVM symbols imported to avoid polluting including translation units.
using llvm::PrintFatalError;
using llvm::Record;
using llvm::RecordKeeper;
using llvm::SmallVector;
using llvm::StringRef;

// Classification of platform support descriptors.
enum class PlatformSupportKind {
  All,
  NotSupported,
  InheritFromExtension,
  CoreChildOf,
  ExactCoreFamily,
  ProductChildOf,
  ExactPlatform,
  InGroup,
  AnyOf,
  AllOf,
  Not,
  Unknown
};

// Capability entry for an extension.
struct CapabilityEntry {
  StringRef Name;
  const Record *Root; // original capability record
  const Record *ProductionSupport = nullptr;
  const Record *ExperimentalSupport = nullptr;
};

struct ExtensionEntry {
  StringRef Name;
  StringRef SpecURL;
  const Record *Root; // original extension record (first encountered)
  const Record *ProductionSupport = nullptr;
  const Record *ExperimentalSupport = nullptr;
  SmallVector<CapabilityEntry, 8> Capabilities;
};

// Container alias for list of extensions
using SPIRVExtensions = SmallVector<ExtensionEntry, 64>;

inline PlatformSupportKind classifyPlatformSupport(const Record *R) {
  StringRef Name = R->getName();
  if (Name == "AllPlatformSupport")
    return PlatformSupportKind::All;
  if (Name == "NotSupported")
    return PlatformSupportKind::NotSupported;
  if (Name == "InheritFromExtension")
    return PlatformSupportKind::InheritFromExtension;
  if (R->isSubClassOf("isCoreChildOf"))
    return PlatformSupportKind::CoreChildOf;
  if (R->isSubClassOf("ExactCoreFamily"))
    return PlatformSupportKind::ExactCoreFamily;
  if (R->isSubClassOf("isProductChildOf"))
    return PlatformSupportKind::ProductChildOf;
  if (R->isSubClassOf("ExactPlatform"))
    return PlatformSupportKind::ExactPlatform;
  if (R->isSubClassOf("isInGroup"))
    return PlatformSupportKind::InGroup;
  if (R->isSubClassOf("AnyOf"))
    return PlatformSupportKind::AnyOf;
  if (R->isSubClassOf("AllOf"))
    return PlatformSupportKind::AllOf;
  if (R->isSubClassOf("Not"))
    return PlatformSupportKind::Not;
  return PlatformSupportKind::Unknown;
}

static void validateExtensionPlatformSupport(const Record *Ext) {
  const Record *ExtSupport = Ext->getValueAsDef("ExtSupport");
  const StringRef ExtSupportName = ExtSupport->getName();
  const StringRef ExtName = Ext->getValueAsString("ExtName");
  const bool IsAggregateMode = (ExtSupportName == "InheritFromCapabilities");

  if (ExtSupportName == "InheritFromExtension") {
    PrintFatalError(
        Ext->getLoc(),
        "Extension '" + ExtName.str() +
            "' cannot use InheritFromExtension; specify explicit platform support or InheritFromCapabilities.");
  }

  auto Caps = Ext->getValueAsListOfDefs("ExtCapabilities");
  for (const Record *Cap : Caps) {
    const Record *CapSupport = Cap->getValueAsDef("Support");
    const StringRef CapSupportName = CapSupport->getName();
    const StringRef CapName = Cap->getValueAsString("Name");

    if (CapSupportName == "InheritFromCapabilities") {
      PrintFatalError(Cap->getLoc(), "Capability '" + CapName + "' in extension '" + ExtName +
                                         "' cannot use InheritFromCapabilities.");
    }
    if (IsAggregateMode) {
      if (CapSupportName == "InheritFromExtension") {
        PrintFatalError(Cap->getLoc(), "Capability '" + CapName + "' in extension '" + ExtName +
                                           "' must specify explicit platform support.");
      }
    } else {
      if (CapSupportName != "InheritFromExtension") {
        PrintFatalError(Cap->getLoc(),
                        "Capability '" + CapName + "' in extension '" + ExtName +
                            "' sets explicit Support; set extension platform support to InheritFromCapabilities.");
      }
    }
  }
}


// Validate duplicate extension definitions policy: at most two per name,
// and if two exist, exactly one must be Experimental and one Production.
static void validateExtensionMultiplicityPolicy(const RecordKeeper &Records) {
  auto AllExtensions = Records.getAllDerivedDefinitions("SPIRVExtension");
  llvm::StringMap<SmallVector<const Record *, 4>> Groups;
  for (const Record *Ext : AllExtensions) {
    StringRef Name = Ext->getValueAsString("ExtName");
    Groups[Name].push_back(Ext);
  }

  for (auto &KV : Groups) {
    const StringRef ExtName = KV.first();
    auto &Defs = KV.second;
    if (Defs.size() > 2) {
      // Report error on the third definition location as a representative.
      PrintFatalError(
          Defs[2]->getLoc(),
          "Extension '" + ExtName.str() +
              "' defined more than twice; only one production and one experimental definition are allowed.");
    }
    if (Defs.size() == 2) {
      bool e0 = Defs[0]->getValueAsBit("Experimental");
      bool e1 = Defs[1]->getValueAsBit("Experimental");
      if (e0 == e1) {
        // Both production or both experimental — invalid.
        PrintFatalError(Defs[1]->getLoc(), "Extension '" + ExtName.str() +
                                               "' has two definitions with the same Experimental flag; one must be "
                                               "marked experimental and the other production.");
      }
    }
  }
}

static SPIRVExtensions collectSPIRVExtensionSupport(const RecordKeeper &Records) {
  // Enforce duplicate-definition policy before merging.
  validateExtensionMultiplicityPolicy(Records);
  SPIRVExtensions Result;
  auto AllExtensions = Records.getAllDerivedDefinitions("SPIRVExtension");
  Result.reserve(AllExtensions.size());
  llvm::StringMap<size_t> ExtIndex; // fast lookup of merged extension entries by name

  for (const Record *Ext : AllExtensions) {
    validateExtensionPlatformSupport(Ext);

    StringRef ExtName = Ext->getValueAsString("ExtName");
    const Record *ExtSupport = Ext->getValueAsDef("ExtSupport");
    bool IsExperimental = Ext->getValueAsBit("Experimental");

    // Look up or create merged extension entry via map
    if (ExtIndex.find(ExtName) == ExtIndex.end()) {
      ExtIndex[ExtName] = Result.size();
      Result.push_back(ExtensionEntry{});
      Result.back().Name = ExtName;
      Result.back().SpecURL = Ext->getValueAsString("ExtSpecURL");
      Result.back().Root = Ext;
    }
    ExtensionEntry &Entry = Result[ExtIndex[ExtName]];

    // Assign variant-specific platform support
    (IsExperimental ? Entry.ExperimentalSupport : Entry.ProductionSupport) = ExtSupport;

    // Merge capabilities by name and set variant-specific effective platform
    bool ExtAggregateMode = (ExtSupport->getName() == "InheritFromCapabilities");
    for (const Record *Cap : Ext->getValueAsListOfDefs("ExtCapabilities")) {
      StringRef CapName = Cap->getValueAsString("Name");
      CapabilityEntry *CapEntry = nullptr;
      for (auto &CE : Entry.Capabilities) {
        if (CE.Name == CapName) {
          CapEntry = &CE;
          break;
        }
      }
      if (!CapEntry) {
        Entry.Capabilities.push_back(CapabilityEntry{});
        CapEntry = &Entry.Capabilities.back();
        CapEntry->Name = CapName;
        CapEntry->Root = Cap;
      }

      const Record *CapSupport = Cap->getValueAsDef("Support");
      const Record *Effective =
          (CapSupport->getName() == "InheritFromExtension" && !ExtAggregateMode) ? ExtSupport : CapSupport;

      (IsExperimental ? CapEntry->ExperimentalSupport : CapEntry->ProductionSupport) = Effective;
    }
  }
  return Result;
}

} // namespace IGCCSPIRVPlatformSupport
