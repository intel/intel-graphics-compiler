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
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/Error.h"

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
  ExactPlatform,
  InGroup,
  AnyOf,
  Unknown
};

// Capability entry for an extension.
struct CapabilityEntry {
  StringRef Name;
  const Record *Root;              // original capability record
  const Record *EffectivePlatform; // resolved support (extension or capability)
};

struct ExtensionEntry {
  StringRef Name;
  StringRef SpecURL;
  const Record *Root;                 // original extension record
  const Record *Platforms;            // extension support record (raw)
  bool IsInheritFromCapabilitiesMode; // true if extSupport == InheritFromCapabilities (aggregated capability mode)
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
  if (R->isSubClassOf("ExactPlatform"))
    return PlatformSupportKind::ExactPlatform;
  if (R->isSubClassOf("isInGroup"))
    return PlatformSupportKind::InGroup;
  if (R->isSubClassOf("AnyOf"))
    return PlatformSupportKind::AnyOf;
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

static SPIRVExtensions collectSPIRVExtensionSupport(const RecordKeeper &Records) {
  SPIRVExtensions Result;
  auto AllExtensions = Records.getAllDerivedDefinitions("SPIRVExtension");
  Result.reserve(AllExtensions.size());
  for (const Record *Ext : AllExtensions) {
    validateExtensionPlatformSupport(Ext);
    ExtensionEntry Entry;
    Entry.Name = Ext->getValueAsString("ExtName");
    Entry.SpecURL = Ext->getValueAsString("ExtSpecURL");
    Entry.Root = Ext;
    Entry.Platforms = Ext->getValueAsDef("ExtSupport");
    Entry.IsInheritFromCapabilitiesMode = Entry.Platforms->getName() == "InheritFromCapabilities";
    auto Caps = Ext->getValueAsListOfDefs("ExtCapabilities");
    for (const Record *Cap : Caps) {
      CapabilityEntry CapEntry;
      CapEntry.Name = Cap->getValueAsString("Name");
      CapEntry.Root = Cap;
      const Record *CapSupport = Cap->getValueAsDef("Support");
      if (CapSupport->getName() == "InheritFromExtension" && Entry.Platforms->getName() != "InheritFromCapabilities") {
        CapEntry.EffectivePlatform = Entry.Platforms;
      } else {
        CapEntry.EffectivePlatform = CapSupport; // explicit capability support or InheritFromCapabilities mode
      }
      Entry.Capabilities.push_back(CapEntry);
    }
    Result.push_back(std::move(Entry));
  }
  return Result;
}

} // namespace IGCCSPIRVPlatformSupport
