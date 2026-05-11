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

  void initLookups() {
    for (const Row &R : Rows) {
      NameToProduct.insert({R.Name, R.Product});
      NameToCore.insert({R.CoreName, R.RenderCore});
      ProductToName.insert({R.Product, R.Name});
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

  // Drops the "IGFX_" prefix for markdown readability; pass-through otherwise.
  static StringRef dropIgfxPrefix(StringRef Name) {
    Name.consume_front("IGFX_");
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
        Names.push_back(dropIgfxPrefix(*Name).str());
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

  // Resolve a CoreChildOf or ProductChildOf support record into a
  // human-readable stem (e.g. "XE3P+") and expanded product list.
  // Returns {Stem, Expansion}. Both empty when the support kind is
  // not a core/product range.
  struct StemExpansion {
    std::string Stem;
    std::string Expansion;
  };
  StemExpansion formatStemWithExpansion(const Record *Support,
                                        const std::set<PRODUCT_FAMILY> *Excluded = nullptr) const {
    switch (classifyPlatformSupport(Support)) {
    case PlatformSupportKind::CoreChildOf: {
      StringRef N = Support->getValueAsDef("BaseCore")->getValueAsString("RenderCoreFamily");
      StringRef Short = dropIgfxPrefix(N);
      Short.consume_back("_CORE");
      std::string Stem = (Short + "+").str();
      std::string Exp;
      if (auto CF = lookupCoreFamily(N))
        Exp = expandProductsForCore(*CF, Excluded);
      return {Stem, Exp};
    }
    case PlatformSupportKind::ProductChildOf: {
      StringRef N = Support->getValueAsDef("BasePlatform")->getValueAsString("ProductFamily");
      std::string Stem = (dropIgfxPrefix(N) + "+").str();
      std::string Exp;
      if (auto PF = lookupProductFamily(N))
        Exp = expandProductsForProduct(*PF, Excluded);
      return {Stem, Exp};
    }
    default:
      return {};
    }
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
  void accumulatePlatformTokens(const Record *Support, std::set<std::string> &Tokens, bool &HasAll);
  std::string formatAggregatedTokens(const std::set<std::string> &Tokens, bool HasAll) const;
  bool tryFormatAllOfExclusion(const Record *Support, std::set<std::string> &Tokens, bool &HasAll);
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
  PlatformSupportKind K = classifyPlatformSupport(Support);
  switch (K) {
  case PlatformSupportKind::All:
    return "All platforms";
  case PlatformSupportKind::NotSupported:
    return "Not supported";
  case PlatformSupportKind::InheritFromExtension:
    return "ERROR: Inheritance not resolved - bug in generator";
  case PlatformSupportKind::Unknown:
    return "Unknown platform support";
  default:
    break; // aggregate below
  }

  bool HasAll = false;
  std::set<std::string> Tokens;
  accumulatePlatformTokens(Support, Tokens, HasAll);
  return formatAggregatedTokens(Tokens, HasAll);
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

std::string SPIRVSupportDocsEmitter::formatAggregatedTokens(const std::set<std::string> &Tokens, bool HasAll) const {
  if (HasAll || Tokens.empty())
    return "All platforms";
  std::string Out;
  for (const auto &Token : Tokens) {
    if (!Out.empty())
      Out += ", ";
    Out += Token;
  }
  return Out;
}

bool SPIRVSupportDocsEmitter::tryFormatAllOfExclusion(const Record *Support, std::set<std::string> &Tokens,
                                                      bool &HasAll) {
  auto Conds = Support->getValueAsListOfDefs("Conditions");
  if (Conds.size() < 2)
    return false;

  const Record *Base = nullptr;
  std::vector<std::string> ExcludedNames;
  for (const Record *C : Conds) {
    if (classifyPlatformSupport(C) == PlatformSupportKind::Not) {
      const Record *Inner = C->getValueAsDef("Condition");
      if (classifyPlatformSupport(Inner) != PlatformSupportKind::ExactPlatform)
        return false;
      ExcludedNames.push_back(Inner->getValueAsDef("TargetPlatform")->getValueAsString("ProductFamily").str());
    } else if (!Base) {
      Base = C;
    } else {
      return false; // more than one positive condition
    }
  }
  if (!Base || ExcludedNames.empty())
    return false;

  // Resolve the positive base into a "<stem>" + filtered product expansion.
  std::set<PRODUCT_FAMILY> ExcludedValues;
  for (const std::string &N : ExcludedNames)
    if (auto V = Table.lookupProductFamily(N))
      ExcludedValues.insert(*V);

  auto SE = Table.formatStemWithExpansion(Base, &ExcludedValues);
  std::string Stem, Expansion;
  if (!SE.Stem.empty()) {
    Stem = SE.Stem;
    Expansion = SE.Expansion;
  } else if (classifyPlatformSupport(Base) == PlatformSupportKind::All) {
    Stem = "All platforms";
  } else {
    return false; // let caller fall back to generic AND formatting
  }

  std::vector<std::string> ExcludedDisplay;
  ExcludedDisplay.reserve(ExcludedNames.size());
  for (const std::string &N : ExcludedNames)
    ExcludedDisplay.push_back(ProductCoreTable::dropIgfxPrefix(N).str());

  std::string Rule = Stem + " except " + llvm::join(ExcludedDisplay, ", ");
  std::string Token = !Expansion.empty() ? Rule + " (" + Expansion + ")" : Rule;
  Tokens.insert(std::move(Token));
  return true;
}

void SPIRVSupportDocsEmitter::accumulatePlatformTokens(const Record *Support, std::set<std::string> &Tokens,
                                                       bool &HasAll) {
  switch (classifyPlatformSupport(Support)) {
  case PlatformSupportKind::All:
    HasAll = true;
    return;
  case PlatformSupportKind::NotSupported:
  case PlatformSupportKind::InheritFromExtension:
    return; // no contribution
  case PlatformSupportKind::CoreChildOf:
  case PlatformSupportKind::ProductChildOf: {
    auto [Stem, Products] = Table.formatStemWithExpansion(Support);
    Tokens.insert(!Products.empty() ? Stem + " (" + Products + ")" : Stem);
    return;
  }
  case PlatformSupportKind::ExactCoreFamily: {
    const Record *Core = Support->getValueAsDef("TargetCore");
    std::string Token = ProductCoreTable::dropIgfxPrefix(Core->getValueAsString("RenderCoreFamily")).str();
    Tokens.insert(Token);
    return;
  }
  case PlatformSupportKind::ExactPlatform: {
    const Record *Plat = Support->getValueAsDef("TargetPlatform");
    std::string Token = ProductCoreTable::dropIgfxPrefix(Plat->getValueAsString("ProductFamily")).str();
    Tokens.insert(Token);
    return;
  }
  case PlatformSupportKind::InGroup: {
    const Record *Group = Support->getValueAsDef("TargetGroup");
    auto Ps = Group->getValueAsListOfDefs("Platforms");
    for (const Record *P : Ps) {
      std::string Token = ProductCoreTable::dropIgfxPrefix(P->getValueAsString("ProductFamily")).str();
      Tokens.insert(Token);
    }
    return;
  }
  case PlatformSupportKind::AnyOf: {
    auto Conds = Support->getValueAsListOfDefs("Conditions");
    for (const Record *C : Conds)
      accumulatePlatformTokens(C, Tokens, HasAll);
    return;
  }
  case PlatformSupportKind::AllOf: {
    // Try smart exclusion pattern first
    if (tryFormatAllOfExclusion(Support, Tokens, HasAll))
      return;

    // Otherwise, standard AND formatting
    auto Conds = Support->getValueAsListOfDefs("Conditions");
    std::set<std::string> SubTokens;
    bool SubHasAll = false;
    for (const Record *C : Conds) {
      accumulatePlatformTokens(C, SubTokens, SubHasAll);
    }
    if (SubHasAll) {
      HasAll = true;
      return;
    }
    if (!SubTokens.empty()) {
      std::string CombinedToken = "(";
      bool First = true;
      for (const auto &T : SubTokens) {
        if (!First)
          CombinedToken += " AND ";
        CombinedToken += T;
        First = false;
      }
      CombinedToken += ")";
      Tokens.insert(CombinedToken);
    }
    return;
  }
  case PlatformSupportKind::Not: {
    const Record *InnerCond = Support->getValueAsDef("Condition");
    std::set<std::string> SubTokens;
    bool SubHasAll = false;
    accumulatePlatformTokens(InnerCond, SubTokens, SubHasAll);
    if (SubHasAll) {
      // NOT(All) means nothing is supported
      return;
    }
    for (const auto &T : SubTokens) {
      Tokens.insert("NOT " + T);
    }
    return;
  }
  case PlatformSupportKind::Unknown:
    return; // ignore unknown in aggregation
  }
}
// ===== SPIRVSupportDocsEmitter implementation end =====

// ===== SPIRVSupportQueriesEmitter implementation begin =====

// Builds the SPIRVExtensionsSupport.h header during the IGC build.
// The generated header is consumed by other translation units to:
//  - Emit YAML passed to Neo describing per-platform SPIR-V support.
//  - Inform the SPIRV-LLVM Translator which extensions are accepted so it can reject unsupported ones early.
class SPIRVSupportQueriesEmitter {
  const SPIRVExtensions &Extensions;
  void emitSPIRVExtensionStructures(raw_ostream &OS);
  void emitPlatformSupportQuery(raw_ostream &OS);
  void emitForwardDecls(raw_ostream &OS);
  void emitExtensionsVector(raw_ostream &OS);
  void emitExtensionSupportFn(raw_ostream &OS);
  void emitCapabilitySupportFn(raw_ostream &OS);
  void emitGetSupportedInfoFn(raw_ostream &OS);
  std::string buildPredicate(const Record *Support, StringRef platformVar);
  void emitSingleExtensionSupportIf(raw_ostream &OS, const ExtensionEntry &Ext);
  void emitSingleCapabilitySupportIf(raw_ostream &OS, const CapabilityEntry &Cap);
  std::string buildAutoExtensionPredicate(const ExtensionEntry &Ext);

public:
  SPIRVSupportQueriesEmitter(const SPIRVExtensions &M) : Extensions(M) {}
  void emit(raw_ostream &OS);
};

void SPIRVSupportQueriesEmitter::emit(raw_ostream &OS) {
  OS << "#ifndef IGCC_SPIRV_EXTENSIONS_SUPPORT_H\n";
  OS << "#define IGCC_SPIRV_EXTENSIONS_SUPPORT_H\n\n";
  OS << "#include <vector>\n";
  OS << "#include <string>\n";
  OS << "#include \"igfxfmid.h\"\n\n";
  OS << "namespace IGC {\n";
  OS << "namespace SPIRVExtensionsSupport {\n\n";
  emitSPIRVExtensionStructures(OS);
  emitPlatformSupportQuery(OS);
  OS << "} // namespace SPIRVExtensionsSupport\n";
  OS << "} // namespace IGC\n\n";
  OS << "#endif // IGCC_SPIRV_EXTENSIONS_SUPPORT_H\n";
}

void SPIRVSupportQueriesEmitter::emitSPIRVExtensionStructures(raw_ostream &OS) {
  OS << "// Helper function for core family hierarchy checks\n";
  OS << "inline bool isCoreChildOf(PLATFORM Platform, GFXCORE_FAMILY Core) {\n";
  OS << "  return Platform.eRenderCoreFamily >= Core;\n";
  OS << "}\n\n";
  OS << "// Helper function for product family hierarchy checks\n";
  OS << "inline bool isProductChildOf(PLATFORM Platform, PRODUCT_FAMILY Product) {\n";
  OS << "  return Platform.eProductFamily >= Product;\n";
  OS << "}\n\n";
  OS << "// SPIR-V Extension and Capability structures\n";
  OS << "struct SPIRVCapability {\n";
  OS << "  std::string Name;\n";
  OS << "};\n\n";
  OS << "struct SPIRVExtension {\n";
  OS << "  std::string Name;\n";
  OS << "  std::string SpecURL;\n";
  OS << "  std::vector<SPIRVCapability> Capabilities;\n";
  OS << "};\n\n";
}

void SPIRVSupportQueriesEmitter::emitForwardDecls(raw_ostream &OS) {
  OS << "// Forward declarations\n";
  OS << "inline bool isExtensionSupported(const std::string& ExtensionName, PLATFORM Platform, bool "
        "includeExperimental);\n";
  OS << "inline bool isCapabilitySupported(const std::string& CapabilityName, PLATFORM Platform, bool "
        "includeExperimental);\n\n";
}

void SPIRVSupportQueriesEmitter::emitExtensionsVector(raw_ostream &OS) {
  OS << "// Static vector of all defined extensions with their capabilities\n";
  OS << "static const std::vector<SPIRVExtension> AllExtensions = {\n";
  for (auto It = Extensions.begin(); It != Extensions.end(); ++It) {
    const ExtensionEntry &Ext = *It;
    OS << "  {\n";
    OS << "    \"" << Ext.Name << "\",\n";
    OS << "    \"" << Ext.SpecURL << "\",\n";
    OS << "    {\n";
    for (auto CapIt = Ext.Capabilities.begin(); CapIt != Ext.Capabilities.end(); ++CapIt) {
      OS << "      {\"" << CapIt->Name << "\"}";
      if (std::next(CapIt) != Ext.Capabilities.end())
        OS << ",";
      OS << "\n";
    }
    OS << "    }\n";
    OS << "  }" << (std::next(It) != Extensions.end() ? "," : "") << "\n";
  }
  OS << "};\n\n";
}

std::string SPIRVSupportQueriesEmitter::buildAutoExtensionPredicate(const ExtensionEntry &Ext) {
  if (Ext.Capabilities.empty())
    return "false /* No capabilities defined */";
  std::string Expr;
  bool First = true;
  for (const auto &Cap : Ext.Capabilities) {
    if (!First)
      Expr += " || ";
    Expr += "isCapabilitySupported(\"" + Cap.Name.str() + "\", Platform, includeExperimental)";
    First = false;
  }
  return Expr;
}

void SPIRVSupportQueriesEmitter::emitSingleExtensionSupportIf(raw_ostream &OS, const ExtensionEntry &Ext) {
  OS << "  if (ExtensionName == \"" << Ext.Name << "\") {\n";
  // Experimental variant (gated by includeExperimental)
  if (Ext.ExperimentalSupport) {
    OS << "    if (includeExperimental) {\n";
    if (Ext.ExperimentalSupport->getName() == "InheritFromCapabilities") {
      OS << "      if (" << buildAutoExtensionPredicate(Ext) << ") return true;\n";
    } else {
      OS << "      if (" << buildPredicate(Ext.ExperimentalSupport, "Platform") << ") return true;\n";
    }
    OS << "    }\n";
  }
  // Production variant
  if (Ext.ProductionSupport) {
    if (Ext.ProductionSupport->getName() == "InheritFromCapabilities") {
      OS << "    if (" << buildAutoExtensionPredicate(Ext) << ") return true;\n";
    } else {
      OS << "    if (" << buildPredicate(Ext.ProductionSupport, "Platform") << ") return true;\n";
    }
  }
  OS << "  }\n";
}

void SPIRVSupportQueriesEmitter::emitSingleCapabilitySupportIf(raw_ostream &OS, const CapabilityEntry &Cap) {
  OS << "  if (CapabilityName == \"" << Cap.Name << "\") {\n";
  if (Cap.ExperimentalSupport) {
    OS << "    if (includeExperimental) {\n";
    OS << "      if (" << buildPredicate(Cap.ExperimentalSupport, "Platform") << ") return true;\n";
    OS << "    }\n";
  }
  if (Cap.ProductionSupport) {
    OS << "    if (" << buildPredicate(Cap.ProductionSupport, "Platform") << ") return true;\n";
  }
  OS << "  }\n";
}

void SPIRVSupportQueriesEmitter::emitExtensionSupportFn(raw_ostream &OS) {
  OS << "// Individual extension/capability query functions\n";
  OS << "inline bool isExtensionSupported(const std::string& ExtensionName, PLATFORM Platform, bool "
        "includeExperimental) {\n";
  for (const auto &Ext : Extensions)
    emitSingleExtensionSupportIf(OS, Ext);
  OS << "  return false;\n";
  OS << "}\n\n";
}

void SPIRVSupportQueriesEmitter::emitCapabilitySupportFn(raw_ostream &OS) {
  OS << "inline bool isCapabilitySupported(const std::string& CapabilityName, PLATFORM Platform, bool "
        "includeExperimental) {\n";
  for (const auto &Ext : Extensions) {
    for (const auto &Cap : Ext.Capabilities) {
      emitSingleCapabilitySupportIf(OS, Cap);
    }
  }
  OS << "  return false;\n";
  OS << "}\n\n";
}

void SPIRVSupportQueriesEmitter::emitGetSupportedInfoFn(raw_ostream &OS) {
  OS << "// Get extension info with capabilities for a platform\n";
  OS << "inline std::vector<SPIRVExtension> getSupportedExtensionInfo(PLATFORM Platform, bool includeExperimental = "
        "false) {\n";
  OS << "  std::vector<SPIRVExtension> SupportedExtensions;\n";
  OS << "  for (const auto& Ext : AllExtensions) {\n";
  OS << "    if (isExtensionSupported(Ext.Name, Platform, includeExperimental)) {\n";
  OS << "      SPIRVExtension SupportedExt;\n";
  OS << "      SupportedExt.Name = Ext.Name;\n";
  OS << "      SupportedExt.SpecURL = Ext.SpecURL;\n";
  OS << "      for (const auto& Cap : Ext.Capabilities) {\n";
  OS << "        if (isCapabilitySupported(Cap.Name, Platform, includeExperimental)) {\n";
  OS << "          SupportedExt.Capabilities.push_back(Cap);\n";
  OS << "        }\n";
  OS << "      }\n";
  OS << "      SupportedExtensions.push_back(SupportedExt);\n";
  OS << "    }\n";
  OS << "  }\n";
  OS << "  return SupportedExtensions;\n";
  OS << "}\n";
}

void SPIRVSupportQueriesEmitter::emitPlatformSupportQuery(raw_ostream &OS) {
  emitForwardDecls(OS);
  emitExtensionsVector(OS);
  emitExtensionSupportFn(OS);
  emitCapabilitySupportFn(OS);
  emitGetSupportedInfoFn(OS);
}

std::string SPIRVSupportQueriesEmitter::buildPredicate(const Record *Support, StringRef PlatformVar) {
  switch (classifyPlatformSupport(Support)) {
  case PlatformSupportKind::NotSupported:
    return "false /* Not supported */";
  case PlatformSupportKind::InheritFromExtension:
    return "false /* ERROR: InheritFromExtension should have been resolved - this is a bug */";
  case PlatformSupportKind::CoreChildOf: {
    const Record *BaseCore = Support->getValueAsDef("BaseCore");
    StringRef BaseRenderCoreFamily = BaseCore->getValueAsString("RenderCoreFamily");
    return (Twine("isCoreChildOf(" + PlatformVar + ", ") + BaseRenderCoreFamily + ")").str();
  }
  case PlatformSupportKind::ExactCoreFamily: {
    const Record *Core = Support->getValueAsDef("TargetCore");
    StringRef CoreFamily = Core->getValueAsString("RenderCoreFamily");
    return (Twine(PlatformVar) + ".eRenderCoreFamily == " + CoreFamily).str();
  }
  case PlatformSupportKind::ProductChildOf: {
    const Record *BasePlatform = Support->getValueAsDef("BasePlatform");
    StringRef BaseProductFamily = BasePlatform->getValueAsString("ProductFamily");
    return (Twine("isProductChildOf(" + PlatformVar + ", ") + BaseProductFamily + ")").str();
  }
  case PlatformSupportKind::ExactPlatform: {
    const Record *Platform = Support->getValueAsDef("TargetPlatform");
    StringRef ProductFamily = Platform->getValueAsString("ProductFamily");
    return (Twine(PlatformVar) + ".eProductFamily == " + ProductFamily).str();
  }
  case PlatformSupportKind::InGroup: {
    const Record *Group = Support->getValueAsDef("TargetGroup");
    auto Platforms = Group->getValueAsListOfDefs("Platforms");
    std::string expr = "(";
    bool first = true;
    for (const Record *P : Platforms) {
      if (!first)
        expr += " || ";
      expr += (Twine(PlatformVar) + ".eProductFamily == " + P->getValueAsString("ProductFamily")).str();
      first = false;
    }
    expr += ")";
    return expr;
  }
  case PlatformSupportKind::AnyOf: {
    auto Conditions = Support->getValueAsListOfDefs("Conditions");
    std::string Expr;
    bool First = true;
    for (const Record *Cond : Conditions) {
      if (!First)
        Expr += " || ";
      Expr += "(" + buildPredicate(Cond, PlatformVar) + ")";
      First = false;
    }
    if (Expr.empty())
      Expr = "false /* Empty AnyOf */";
    return Expr;
  }
  case PlatformSupportKind::AllOf: {
    auto Conditions = Support->getValueAsListOfDefs("Conditions");
    std::string Expr;
    bool First = true;
    for (const Record *Cond : Conditions) {
      if (!First)
        Expr += " && ";
      Expr += "(" + buildPredicate(Cond, PlatformVar) + ")";
      First = false;
    }
    if (Expr.empty())
      Expr = "true /* Empty AllOf */";
    return Expr;
  }
  case PlatformSupportKind::Not: {
    const Record *InnerCond = Support->getValueAsDef("Condition");
    return "!(" + buildPredicate(InnerCond, PlatformVar) + ")";
  }
  case PlatformSupportKind::All:
    return "true /* Supported on all platforms */";
  case PlatformSupportKind::Unknown:
    return "true /* Supported on all platforms (unknown treated as all) */";
  }
  return "true";
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

cl::opt<ActionType> Action(
    cl::desc("Action to perform:"),
    cl::values(clEnumValN(EmitSPIRVDocs, "gen-igcc-spirv-extensions-docs",
                          "Generate IGCCompute supported SPIR-V extensions documentation"),
               clEnumValN(EmitOptionsDocs, "gen-igcc-options-docs",
                          "Generate IGCCompute supported options documentation"),
               clEnumValN(EmitSPIRVExtensionSupportHeader, "gen-igcc-spirv-extension-support-header",
                          "Generate IGCCompute SPIR-V extension support query header (structures + query functions)")));
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
