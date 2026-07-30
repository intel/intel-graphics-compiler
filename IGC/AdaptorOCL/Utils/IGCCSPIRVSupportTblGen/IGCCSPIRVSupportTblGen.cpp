/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
//===----------------------------------------------------------------------===//
//
/// \file
/// TableGen backend for supported SPIR-V extensions & capabilities.
///
/// Responsibilities:
/// 1. Generate C++ structures plus query functions describing supported
///    SPIR-V extensions and their capabilities.
/// 2. Generate Markdown documentation for supported SPIR-V extensions and capabilities.
///
//===----------------------------------------------------------------------===//
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/ADT/SmallSet.h"

#include "IGCCSPIRVPlatformSupport.h"

// PRODUCT_FAMILY / GFXCORE_FAMILY enums for the markdown emitter.
#include "inc/common/igfxfmid.h"

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <vector>

using namespace llvm;
using namespace std;

#define DEBUG_TYPE "IGCCSPIRVSupportTblGen"

using IGCCSPIRVPlatformSupport::CapabilityEntry;
using IGCCSPIRVPlatformSupport::classifyPlatformSupport;
using IGCCSPIRVPlatformSupport::collectSPIRVExtensionSupport;
using IGCCSPIRVPlatformSupport::ExtensionEntry;
using IGCCSPIRVPlatformSupport::PlatformSupportKind;
using IGCCSPIRVPlatformSupport::SPIRVExtensions;

// Platform/core-family expansion for the markdown emitter. Turns TD tokens
// like "IGFX_XE3P_CORE and newer" into concrete product lists.
class ProductCoreTable {
  struct Row {
    StringRef Name;
    PRODUCT_FAMILY Product;
    StringRef CoreName;
    GFXCORE_FAMILY RenderCore;
  };

  // Static data shared between all instances.
#define PCR(P, C) {#P, P, #C, C}
  // clang-format off
  static constexpr Row kPlatforms[] = {
      PCR(IGFX_TIGERLAKE_LP,  IGFX_GEN12_CORE),
      PCR(IGFX_DG1,           IGFX_GEN12_CORE),
      PCR(IGFX_ROCKETLAKE,    IGFX_GEN12_CORE),
      PCR(IGFX_ALDERLAKE_S,   IGFX_GEN12_CORE),
      PCR(IGFX_ALDERLAKE_P,   IGFX_GEN12_CORE),
      PCR(IGFX_ALDERLAKE_N,   IGFX_GEN12_CORE),
      PCR(IGFX_XE_HP_SDV,     IGFX_XE_HP_CORE),
      PCR(IGFX_DG2,           IGFX_XE_HPG_CORE),
      PCR(IGFX_METEORLAKE,    IGFX_XE_HPG_CORE),
      PCR(IGFX_ARROWLAKE,     IGFX_XE_HPG_CORE),
      PCR(IGFX_PVC,           IGFX_XE_HPC_CORE),
      PCR(IGFX_BMG,           IGFX_XE2_HPG_CORE),
      PCR(IGFX_LUNARLAKE,     IGFX_XE2_HPG_CORE),
      PCR(IGFX_PTL,           IGFX_XE3_CORE),
      PCR(IGFX_NVL_XE3G,      IGFX_XE3_CORE),
      PCR(IGFX_NVL,           IGFX_XE3P_CORE),
      PCR(IGFX_CRI,           IGFX_XE3P_CORE),
  };
  // clang-format on
#undef PCR

  SmallVector<Row, 32> Rows;
  StringMap<PRODUCT_FAMILY> NameToProduct;
  StringMap<GFXCORE_FAMILY> NameToCore;
  std::map<PRODUCT_FAMILY, StringRef> ProductToName;
  std::map<GFXCORE_FAMILY, StringRef> CoreToName;

  // Max exclusions before a "<family>+ except ..." stem reads worse than a bare list.
  static constexpr size_t kMaxStemExclusions = 3;

  void initLookups() {
    for (const Row &R : Rows) {
      NameToProduct.insert({R.Name, R.Product});
      NameToCore.insert({R.CoreName, R.RenderCore});
      ProductToName.insert({R.Product, R.Name});
      CoreToName.insert({R.RenderCore, R.CoreName});
    }
  }

  bool evaluateSupportForRow(const Record *Support, const Row &R) const {
    switch (classifyPlatformSupport(Support)) {
    case PlatformSupportKind::All:
      return true;
    case PlatformSupportKind::NotSupported:
    case PlatformSupportKind::InheritFromExtension:
    case PlatformSupportKind::Unknown:
      return false;
    case PlatformSupportKind::CoreChildOf: {
      auto CF = lookupCoreFamily(Support->getValueAsDef("BaseCore")->getValueAsString("RenderCoreFamily"));
      return CF && R.RenderCore >= *CF;
    }
    case PlatformSupportKind::ExactCoreFamily: {
      auto CF = lookupCoreFamily(Support->getValueAsDef("TargetCore")->getValueAsString("RenderCoreFamily"));
      return CF && R.RenderCore == *CF;
    }
    case PlatformSupportKind::ProductChildOf: {
      auto PF = lookupProductFamily(Support->getValueAsDef("BasePlatform")->getValueAsString("ProductFamily"));
      return PF && R.Product >= *PF;
    }
    case PlatformSupportKind::ExactPlatform: {
      auto PF = lookupProductFamily(Support->getValueAsDef("TargetPlatform")->getValueAsString("ProductFamily"));
      return PF && R.Product == *PF;
    }
    case PlatformSupportKind::InGroup: {
      auto Ps = Support->getValueAsDef("TargetGroup")->getValueAsListOfDefs("Platforms");
      for (const Record *P : Ps) {
        auto PF = lookupProductFamily(P->getValueAsString("ProductFamily"));
        if (PF && R.Product == *PF)
          return true;
      }
      return false;
    }
    case PlatformSupportKind::AnyOf:
      for (const Record *C : Support->getValueAsListOfDefs("Conditions"))
        if (evaluateSupportForRow(C, R))
          return true;
      return false;
    case PlatformSupportKind::AllOf:
      for (const Record *C : Support->getValueAsListOfDefs("Conditions"))
        if (!evaluateSupportForRow(C, R))
          return false;
      return true;
    case PlatformSupportKind::Not:
      return !evaluateSupportForRow(Support->getValueAsDef("Condition"), R);
    }
    return false;
  }

public:
  ProductCoreTable() {
    Rows.append(std::begin(kPlatforms), std::end(kPlatforms));
    initLookups();
  }

  static StringRef displayProductName(StringRef Name) {
    if (Name == "IGFX_NVL_XE3G")
      return "NVL S";
    if (Name == "IGFX_NVL")
      return "NVL P";
    return dropIgfxPrefix(Name);
  }

  // Drops the "IGFX_" prefix for markdown readability; pass-through otherwise.
  static StringRef dropIgfxPrefix(StringRef Name) {
    (void)Name.consume_front("IGFX_");
    return Name;
  }

  std::optional<PRODUCT_FAMILY> lookupProductFamily(StringRef Name) const {
    auto It = NameToProduct.find(Name);
    if (It == NameToProduct.end())
      return std::nullopt;
    return It->second;
  }

  std::optional<GFXCORE_FAMILY> lookupCoreFamily(StringRef Name) const {
    auto It = NameToCore.find(Name);
    if (It == NameToCore.end())
      return std::nullopt;
    return It->second;
  }

  std::optional<StringRef> stringifyProductFamily(PRODUCT_FAMILY V) const {
    auto It = ProductToName.find(V);
    if (It == ProductToName.end())
      return std::nullopt;
    return It->second;
  }

  // Comma-separated display names for a set of PRODUCT_FAMILY values.
  std::string stringifyPlatforms(const std::set<PRODUCT_FAMILY> &Platforms) const {
    SmallVector<std::string, 8> Names;
    for (PRODUCT_FAMILY PF : Platforms)
      if (auto Name = stringifyProductFamily(PF))
        Names.push_back(displayProductName(*Name).str());
    return llvm::join(Names, ", ");
  }

  // Comma-separated list of matching PRODUCT_FAMILY names, deduped and sorted
  // by enum value, minus Excluded. Empty when nothing matches.
  template <typename Pred>
  std::string expandProducts(Pred KeepRow, const std::set<PRODUCT_FAMILY> *Excluded = nullptr) const {
    std::set<PRODUCT_FAMILY> Matches;
    for (const Row &R : Rows)
      if (KeepRow(R) && (!Excluded || !Excluded->count(R.Product)))
        Matches.insert(R.Product);
    return stringifyPlatforms(Matches);
  }

  std::string expandProductsForCore(GFXCORE_FAMILY T, const std::set<PRODUCT_FAMILY> *Excluded = nullptr) const {
    return expandProducts([T](const Row &R) { return R.RenderCore >= T; }, Excluded);
  }

  std::string expandProductsForProduct(PRODUCT_FAMILY T, const std::set<PRODUCT_FAMILY> *Excluded = nullptr) const {
    return expandProducts([T](const Row &R) { return R.Product >= T; }, Excluded);
  }

  // Renders a platform set for the docs, prepending a "<family>+ [except ...]"
  // stem derived from and verified against the set when it fits.
  std::string describePlatforms(const std::set<PRODUCT_FAMILY> &Supported) const {
    if (Supported.empty())
      return "Not supported";
    if (Supported.size() == ProductToName.size())
      return "All platforms";

    std::string List = stringifyPlatforms(Supported);

    // Everything at or above the oldest core family present in Supported.
    GFXCORE_FAMILY Base = IGFX_MAX_CORE;
    for (const Row &R : Rows)
      if (Supported.count(R.Product) && R.RenderCore < Base)
        Base = R.RenderCore;

    std::set<PRODUCT_FAMILY> Superset;
    for (const Row &R : Rows)
      if (R.RenderCore >= Base)
        Superset.insert(R.Product);

    std::set<PRODUCT_FAMILY> Excluded;
    std::set_difference(Superset.begin(), Superset.end(), Supported.begin(), Supported.end(),
                        std::inserter(Excluded, Excluded.begin()));
    if (Excluded.size() > kMaxStemExclusions)
      return List;

    if (Superset.size() == ProductToName.size())
      return "All platforms except " + stringifyPlatforms(Excluded) + " (" + List + ")";

    auto It = CoreToName.find(Base);
    if (It == CoreToName.end())
      return List;
    StringRef Short = dropIgfxPrefix(It->second);
    Short.consume_back("_CORE");
    std::string Stem = (Short + "+").str();
    if (!Excluded.empty())
      Stem += " except " + stringifyPlatforms(Excluded);
    return Stem + " (" + List + ")";
  }

  // Evaluate a platform-support predicate against all known platforms,
  // returning the concrete set of matching PRODUCT_FAMILY values.
  std::set<PRODUCT_FAMILY> evaluateSupport(const Record *Support) const {
    std::set<PRODUCT_FAMILY> Result;
    for (const Row &R : Rows)
      if (evaluateSupportForRow(Support, R))
        Result.insert(R.Product);
    return Result;
  }
};

// ===== SPIRVSupportDocsEmitter implementation begin =====

// Generates the spirv-supported-extensions.md document listing each supported
// SPIR-V extension, its specification URL, aggregated platform support, and
// capability-level support.
class SPIRVSupportDocsEmitter {
  const SPIRVExtensions &Extensions;
  const ProductCoreTable Table;
  bool shouldShowCapabilityPlatformSupport(const ExtensionEntry &Ext, const CapabilityEntry &Cap) const;
  std::string formatPlatformSupport(const Record *Support);
  std::string formatAdditionalExperimentalPlatforms(const Record *ExpSupport, const Record *ProdSupport);

public:
  SPIRVSupportDocsEmitter(const SPIRVExtensions &M) : Extensions(M), Table() {}
  void emit(raw_ostream &OS);
};

void SPIRVSupportDocsEmitter::emit(raw_ostream &OS) {
  OS << "# Supported SPIR-V Extensions\n\n";
  OS << "This document lists all SPIR-V extensions supported by IGC and their platform requirements.\n\n";
  for (const auto &Ext : Extensions) {
    OS << "## " << Ext.Name << "\n\n";
    bool HasProd = (Ext.ProductionSupport != nullptr);
    bool HasExp = (Ext.ExperimentalSupport != nullptr);
    OS << "**Specification**: " << Ext.SpecURL << "\n\n";
    // Print explicit extension platform support when not inheriting from capabilities
    if (HasProd && Ext.ProductionSupport->getName() != "InheritFromCapabilities") {
      OS << "> **Supported on**: " << formatPlatformSupport(Ext.ProductionSupport) << "\n\n";
    }
    if (HasExp && Ext.ExperimentalSupport->getName() != "InheritFromCapabilities") {
      if (HasProd && Ext.ProductionSupport->getName() != "InheritFromCapabilities") {
        // Subtract officially supported platforms to avoid overlap in the docs.
        std::string ExpOnly = formatAdditionalExperimentalPlatforms(Ext.ExperimentalSupport, Ext.ProductionSupport);
        if (!ExpOnly.empty())
          OS << "> **Additionally experimentally supported on**: " << ExpOnly << "\n\n";
      } else {
        OS << "> **Experimentally supported on**: " << formatPlatformSupport(Ext.ExperimentalSupport) << "\n\n";
      }
    }
    OS << "**Capabilities**:\n\n";
    if (Ext.Capabilities.empty()) {
      OS << "- No capabilities defined\n";
    } else {
      for (const auto &Cap : Ext.Capabilities) {
        OS << "- **" << Cap.Name << "**";
        if (shouldShowCapabilityPlatformSupport(Ext, Cap)) {
          if (Cap.ProductionSupport) {
            OS << "\n  > **Supported On**: " << formatPlatformSupport(Cap.ProductionSupport);
          }
          if (Cap.ExperimentalSupport) {
            OS << "\n  > **Experimentally supported on**: " << formatPlatformSupport(Cap.ExperimentalSupport);
          }
        }
        OS << "\n";
      }
    }
    OS << "\n---\n\n";
  }
}

bool SPIRVSupportDocsEmitter::shouldShowCapabilityPlatformSupport(const ExtensionEntry &Ext,
                                                                  const CapabilityEntry &Cap) const {
  // Show capability platform support if extension inherits from capabilities in any variant,
  // or if the capability specified explicit support (not InheritFromExtension).
  bool ExtInherit = (Ext.ProductionSupport && Ext.ProductionSupport->getName() == "InheritFromCapabilities") ||
                    (Ext.ExperimentalSupport && Ext.ExperimentalSupport->getName() == "InheritFromCapabilities");
  if (ExtInherit)
    return true;
  const Record *OriginalCapSupport = Cap.Root->getValueAsDef("Support");
  return OriginalCapSupport->getName() != "InheritFromExtension";
}

std::string SPIRVSupportDocsEmitter::formatPlatformSupport(const Record *Support) {
  switch (classifyPlatformSupport(Support)) {
  case PlatformSupportKind::All:
    return "All platforms";
  case PlatformSupportKind::NotSupported:
    return "Not supported";
  case PlatformSupportKind::InheritFromExtension:
    return "ERROR: Inheritance not resolved - bug in generator";
  case PlatformSupportKind::Unknown:
    return "Unknown platform support";
  default:
    break;
  }
  return Table.describePlatforms(Table.evaluateSupport(Support));
}

std::string SPIRVSupportDocsEmitter::formatAdditionalExperimentalPlatforms(const Record *ExpSupport,
                                                                           const Record *ProdSupport) {
  auto ExpPlatforms = Table.evaluateSupport(ExpSupport);
  auto ProdPlatforms = Table.evaluateSupport(ProdSupport);
  std::set<PRODUCT_FAMILY> ExpOnly;
  std::set_difference(ExpPlatforms.begin(), ExpPlatforms.end(), ProdPlatforms.begin(), ProdPlatforms.end(),
                      std::inserter(ExpOnly, ExpOnly.begin()));
  if (ExpOnly.empty())
    return "";
  return Table.stringifyPlatforms(ExpOnly);
}

// ===== SPIRVSupportDocsEmitter implementation end =====

// ===== SPIRVSupportQueriesEmitter implementation begin =====

// Emits a .inc file as a nested aggregate initializer fragment — each
// extension entry contains its capabilities inline. The .inc is included
// once into a std::vector<ExtensionDef> brace initializer in the .h header.
//
// Each capability/extension has a single unified predicate that encodes both
// the experimental and production support tiers.
class SPIRVSupportQueriesEmitter {
  const SPIRVExtensions &Extensions;
  void emitExtensionData(raw_ostream &OS);

  // Translate a TD platform-support record into a Pred:: template reference.
  std::string flattenPredicate(const Record *Support);

  // Emit a single tier column: "nullptr" if the tier is absent (no TD record,
  // NotSupported, or InheritFromCapabilities), otherwise the flattened Pred::.
  std::string emitTierColumn(const Record *Support);

  // Format capability ID as a C++ expression (0 if not reported).
  std::string formatCapabilityId(const ExtensionEntry &Ext, const CapabilityEntry &Cap);

public:
  SPIRVSupportQueriesEmitter(const SPIRVExtensions &M) : Extensions(M) {}
  void emit(raw_ostream &OS);
};

void SPIRVSupportQueriesEmitter::emit(raw_ostream &OS) {
  OS << "//===----------------------------------------------------------------------===//\n";
  OS << "// SPIRVExtensionsSupport.inc — generated data for SPIRVExtensionsSupport.h\n";
  OS << "//\n";
  OS << "// DO NOT EDIT. This file is generated by igcc-spirv-support-tblgen.\n";
  OS << "// Included once into a std::vector<ExtensionDef> brace initializer.\n";
  OS << "//===----------------------------------------------------------------------===//\n\n";
  emitExtensionData(OS);
}

// Format capability ID: TD override emits literal, otherwise spv:: reference.
// Returns "0" for capabilities that should not appear in the ID map.
std::string SPIRVSupportQueriesEmitter::formatCapabilityId(const ExtensionEntry &Ext, const CapabilityEntry &Cap) {
  // Only capabilities with production support (in extensions with production
  // support) get a numeric ID reported.
  if (!Ext.ProductionSupport || !Cap.ProductionSupport)
    return "0";
  if (Cap.Id >= 0)
    return (Twine(Cap.Id) + "u").str();
  return ("static_cast<uint32_t>(spv::Capability" + Cap.Name + ")").str();
}

// Emit nested brace initializers — one per extension, with capabilities inline.
// Each entry has two predicate columns: Production and Experimental. nullptr
// in a column means "no support in this tier"; both nullptr at the extension
// level means InheritFromCapabilities.
void SPIRVSupportQueriesEmitter::emitExtensionData(raw_ostream &OS) {
  for (const auto &Ext : Extensions) {
    std::string ExtProd = emitTierColumn(Ext.ProductionSupport);
    std::string ExtExp = emitTierColumn(Ext.ExperimentalSupport);

    OS << "{\"" << Ext.Name << "\", \"" << Ext.SpecURL << "\", " << ExtProd << ", " << ExtExp << ", {";

    if (Ext.Capabilities.empty()) {
      OS << "}},\n";
      continue;
    }

    OS << "\n";
    for (const auto &Cap : Ext.Capabilities) {
      std::string CapProd = emitTierColumn(Cap.ProductionSupport);
      std::string CapExp = emitTierColumn(Cap.ExperimentalSupport);
      std::string CapId = formatCapabilityId(Ext, Cap);
      OS << "  {\"" << Cap.Name << "\", " << CapProd << ", " << CapExp << ", " << CapId << "},\n";
    }
    OS << "}},\n";
  }
}

// Render a single tier predicate column. Absent / NotSupported /
// InheritFromCapabilities all collapse to nullptr — the tier carries no
// platform rule of its own.
std::string SPIRVSupportQueriesEmitter::emitTierColumn(const Record *Support) {
  if (!Support)
    return "nullptr";
  StringRef N = Support->getName();
  if (N == "NotSupported" || N == "InheritFromCapabilities")
    return "nullptr";
  return flattenPredicate(Support);
}

// Returns the TD class/def name driving this PlatformSupport record: the def
// name itself for the singletons (AllPlatformSupport, NotSupported, ...), or
// the immediate base class name otherwise - which covers both anonymous
// instantiations (isCoreChildOf<...>) and named defs (def X : AllOf<[...]>).
static StringRef predicateKind(const Record *R) {
  StringRef Name = R->getName();
  if (Name == "AllPlatformSupport" || Name == "NotSupported" || Name == "InheritFromExtension" ||
      Name == "InheritFromCapabilities")
    return Name;
#if LLVM_VERSION_MAJOR >= 22
  // LLVM 22+: returns ArrayRef<pair<const Record *, SMRange>>.
  auto Sups = R->getDirectSuperClasses();
  return Sups.empty() ? StringRef() : Sups.back().first->getName();
#else
  SmallVector<Record *, 4> Sups;
  R->getDirectSuperClasses(Sups);
  return Sups.empty() ? StringRef() : Sups.back()->getName();
#endif
}

// Translates a TD platform-support record into a Pred:: template reference.
// The mapping is 1:1 pass-through: every TD class name has a same-named
// Pred:: template, so the generated .inc reads like the .td.
//
// Records that should have been resolved during collection
// (InheritFromExtension, InheritFromCapabilities) or unknown classes trigger
// PrintFatalError pointing at the offending TD record's source location.
std::string SPIRVSupportQueriesEmitter::flattenPredicate(const Record *Support) {
  StringRef Kind = predicateKind(Support);

  if (Kind == "AllPlatformSupport" || Kind == "NotSupported")
    return ("Pred::" + Kind).str();

  if (Kind == "isCoreChildOf")
    return ("Pred::isCoreChildOf<" + Support->getValueAsDef("BaseCore")->getValueAsString("RenderCoreFamily") + ">")
        .str();
  if (Kind == "ExactCoreFamily")
    return ("Pred::ExactCoreFamily<" + Support->getValueAsDef("TargetCore")->getValueAsString("RenderCoreFamily") + ">")
        .str();
  if (Kind == "isProductChildOf")
    return ("Pred::isProductChildOf<" + Support->getValueAsDef("BasePlatform")->getValueAsString("ProductFamily") + ">")
        .str();
  if (Kind == "ExactPlatform")
    return ("Pred::ExactPlatform<" + Support->getValueAsDef("TargetPlatform")->getValueAsString("ProductFamily") + ">")
        .str();
  if (Kind == "isInGroup") {
    auto Platforms = Support->getValueAsDef("TargetGroup")->getValueAsListOfDefs("Platforms");
    SmallVector<std::string, 4> Names;
    for (const Record *P : Platforms)
      Names.push_back(P->getValueAsString("ProductFamily").str());
    return "Pred::isInGroup<" + llvm::join(Names, ", ") + ">";
  }
  if (Kind == "AllOf" || Kind == "AnyOf") {
    auto Conditions = Support->getValueAsListOfDefs("Conditions");
    SmallVector<std::string, 4> Subs;
    for (const Record *C : Conditions)
      Subs.push_back(flattenPredicate(C));
    return ("Pred::" + Kind + "<" + llvm::join(Subs, ", ") + ">").str();
  }
  if (Kind == "Not")
    return "Pred::Not<" + flattenPredicate(Support->getValueAsDef("Condition")) + ">";

  if (Kind == "InheritFromExtension" || Kind == "InheritFromCapabilities")
    PrintFatalError(Support->getLoc(),
                    Kind + " reached flattenPredicate; should have been resolved during collection.");

  PrintFatalError(Support->getLoc(),
                  "Unknown PlatformSupport record '" + Support->getName() + "' in flattenPredicate.");
}
// ===== SPIRVSupportQueriesEmitter implementation end =====

static void emitOptionsDocs(const RecordKeeper &Records, raw_ostream &OS) {
  // To be implemented
}

static void emitSPIRVDocs(const RecordKeeper &Records, raw_ostream &OS) {
  SPIRVExtensions Model = collectSPIRVExtensionSupport(Records);
  SPIRVSupportDocsEmitter(Model).emit(OS);
}


static void emitSPIRVExtensionSupportHeader(const RecordKeeper &Records, raw_ostream &OS) {
  SPIRVExtensions Model = collectSPIRVExtensionSupport(Records);
  SPIRVSupportQueriesEmitter(Model).emit(OS);
}

namespace {
enum ActionType {
  EmitSPIRVDocs,
  EmitOptionsDocs,
  EmitSPIRVExtensionSupportHeader
};

cl::opt<ActionType>
    Action(cl::desc("Action to perform:"),
           cl::values(clEnumValN(EmitSPIRVDocs, "gen-igcc-spirv-extensions-docs",
                                 "Generate IGCCompute supported SPIR-V extensions documentation"),
                      clEnumValN(EmitOptionsDocs, "gen-igcc-options-docs",
                                 "Generate IGCCompute supported options documentation"),
                      clEnumValN(EmitSPIRVExtensionSupportHeader, "gen-igcc-spirv-extension-support-header",
                                 "Generate IGCCompute SPIR-V extension support data .inc file")));
} // namespace

#if LLVM_VERSION_MAJOR >= 22
static bool OptionsAndDocsTblgenMain(raw_ostream &OS, const RecordKeeper &Records) {
#else
static bool OptionsAndDocsTblgenMain(raw_ostream &OS, RecordKeeper &Records) {
#endif
  switch (Action) {
  case EmitSPIRVDocs:
    emitSPIRVDocs(Records, OS);
    break;
  case EmitOptionsDocs:
    emitOptionsDocs(Records, OS);
    break;
  case EmitSPIRVExtensionSupportHeader:
    emitSPIRVExtensionSupportHeader(Records, OS);
    break;
  }

  return false;
}

int main(int argc, char **argv) {
  InitLLVM Init(argc, argv);
  cl::ParseCommandLineOptions(argc, argv);
  return TableGenMain(argv[0], &OptionsAndDocsTblgenMain);
}
