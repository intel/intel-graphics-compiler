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
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/ADT/SmallSet.h"

#include "IGCCSPIRVPlatformSupport.h"

using namespace llvm;
using namespace std;

#define DEBUG_TYPE "IGCCSPIRVSupportTblGen"

using IGCCSPIRVPlatformSupport::CapabilityEntry;
using IGCCSPIRVPlatformSupport::classifyPlatformSupport;
using IGCCSPIRVPlatformSupport::collectSPIRVExtensionSupport;
using IGCCSPIRVPlatformSupport::ExtensionEntry;
using IGCCSPIRVPlatformSupport::PlatformSupportKind;
using IGCCSPIRVPlatformSupport::SPIRVExtensions;

// ===== SPIRVSupportDocsEmitter implementation begin =====

// Generates the spirv-supported-extensions.md document listing each supported
// SPIR-V extension, its specification URL, aggregated platform support, and
// capability-level support.
class SPIRVSupportDocsEmitter {
  const SPIRVExtensions &Extensions;
  bool shouldShowCapabilityPlatformSupport(const ExtensionEntry &Ext, const CapabilityEntry &Cap) const;
  std::string formatPlatformSupport(const Record *Support);
  // Aggregates platform support from all capabilities for extensions using InheritFromCapabilities mode.
  std::string formatInheritFromCapabilitiesSupport(const ExtensionEntry &Ext);
  void accumulatePlatformTokens(const Record *Support, std::set<std::string> &Tokens, bool &HasAll);
  std::string formatAggregatedTokens(const std::set<std::string> &Tokens, bool HasAll) const;

public:
  SPIRVSupportDocsEmitter(const SPIRVExtensions &M) : Extensions(M) {}
  void emit(raw_ostream &OS);
};

void SPIRVSupportDocsEmitter::emit(raw_ostream &OS) {
  OS << "# Supported SPIR-V Extensions\n\n";
  OS << "This document lists all SPIR-V extensions supported by IGC and their platform requirements.\n\n";
  for (const auto &Ext : Extensions) {
    OS << "## " << Ext.Name << "\n\n";
    OS << "**Specification:** [" << Ext.SpecURL << "](" << Ext.SpecURL << ")\n\n";
    OS << "**Extension Platform Support:** "
       << (Ext.IsInheritFromCapabilitiesMode ? formatInheritFromCapabilitiesSupport(Ext)
                                             : formatPlatformSupport(Ext.Platforms))
       << "\n\n";
    OS << "**Capabilities:**\n\n";
    if (Ext.Capabilities.empty()) {
      OS << "- No capabilities defined\n";
    } else {
      for (const auto &Cap : Ext.Capabilities) {
        OS << "- **" << Cap.Name << "**";
        if (shouldShowCapabilityPlatformSupport(Ext, Cap))
          OS << "\n  - Platform Support: " << formatPlatformSupport(Cap.EffectivePlatform);
        OS << "\n";
      }
    }
    OS << "\n---\n\n";
  }
}

bool SPIRVSupportDocsEmitter::shouldShowCapabilityPlatformSupport(const ExtensionEntry &Ext,
                                                                  const CapabilityEntry &Cap) const {
  if (Ext.IsInheritFromCapabilitiesMode)
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

std::string SPIRVSupportDocsEmitter::formatInheritFromCapabilitiesSupport(const ExtensionEntry &Ext) {
  bool HasAll = false;
  std::set<std::string> Tokens;
  for (const auto &Cap : Ext.Capabilities) {
    accumulatePlatformTokens(Cap.EffectivePlatform, Tokens, HasAll);
  }
  return formatAggregatedTokens(Tokens, HasAll);
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

void SPIRVSupportDocsEmitter::accumulatePlatformTokens(const Record *Support, std::set<std::string> &Tokens,
                                                       bool &HasAll) {
  switch (classifyPlatformSupport(Support)) {
  case PlatformSupportKind::All:
    HasAll = true;
    return;
  case PlatformSupportKind::NotSupported:
  case PlatformSupportKind::InheritFromExtension:
    return; // no contribution
  case PlatformSupportKind::CoreChildOf: {
    const Record *BaseCore = Support->getValueAsDef("BaseCore");
    std::string Token = (BaseCore->getValueAsString("RenderCoreFamily") + StringRef(" and newer")).str();
    Tokens.insert(Token);
    return;
  }
  case PlatformSupportKind::ExactPlatform: {
    const Record *Plat = Support->getValueAsDef("TargetPlatform");
    std::string Token = Plat->getValueAsString("ProductFamily").str();
    Tokens.insert(Token);
    return;
  }
  case PlatformSupportKind::InGroup: {
    const Record *Group = Support->getValueAsDef("TargetGroup");
    auto Ps = Group->getValueAsListOfDefs("Platforms");
    for (const Record *P : Ps) {
      std::string Token = P->getValueAsString("ProductFamily").str();
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
  void emitSingleCapabilitySupportIf(raw_ostream &OS, StringRef CapName, const Record *Support);
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
  OS << "inline bool isExtensionSupported(const std::string& ExtensionName, PLATFORM Platform);\n";
  OS << "inline bool isCapabilitySupported(const std::string& CapabilityName, PLATFORM Platform);\n\n";
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
    Expr += "isCapabilitySupported(\"" + Cap.Name.str() + "\", Platform)";
    First = false;
  }
  return Expr;
}

void SPIRVSupportQueriesEmitter::emitSingleExtensionSupportIf(raw_ostream &OS, const ExtensionEntry &Ext) {
  OS << "  if (ExtensionName == \"" << Ext.Name << "\") {\n";
  if (Ext.IsInheritFromCapabilitiesMode) {
    OS << "    return " << buildAutoExtensionPredicate(Ext) << ";\n";
  } else {
    OS << "    return " << buildPredicate(Ext.Platforms, "Platform") << ";\n";
  }
  OS << "  }\n";
}

void SPIRVSupportQueriesEmitter::emitSingleCapabilitySupportIf(raw_ostream &OS, StringRef CapName,
                                                               const Record *Support) {
  OS << "  if (CapabilityName == \"" << CapName << "\") {\n";
  OS << "    return " << buildPredicate(Support, "Platform") << ";\n";
  OS << "  }\n";
}

void SPIRVSupportQueriesEmitter::emitExtensionSupportFn(raw_ostream &OS) {
  OS << "// Individual extension/capability query functions\n";
  OS << "inline bool isExtensionSupported(const std::string& ExtensionName, PLATFORM Platform) {\n";
  for (const auto &Ext : Extensions)
    emitSingleExtensionSupportIf(OS, Ext);
  OS << "  return false;\n";
  OS << "}\n\n";
}

void SPIRVSupportQueriesEmitter::emitCapabilitySupportFn(raw_ostream &OS) {
  OS << "inline bool isCapabilitySupported(const std::string& CapabilityName, PLATFORM Platform) {\n";
  SmallSet<StringRef, 32> Seen;
  for (const auto &Ext : Extensions) {
    for (const auto &Cap : Ext.Capabilities) {
      if (!Seen.insert(Cap.Name).second) {
        PrintFatalError(Cap.Root->getLoc(), "Duplicate capability name '" + Cap.Name.str() +
                                                "' encountered in extension '" + Ext.Name.str() +
                                                "'. Capability names must be unique.");
      }
      emitSingleCapabilitySupportIf(OS, Cap.Name, Cap.EffectivePlatform);
    }
  }
  OS << "  return false;\n";
  OS << "}\n\n";
}

void SPIRVSupportQueriesEmitter::emitGetSupportedInfoFn(raw_ostream &OS) {
  OS << "// Get extension info with capabilities for a platform\n";
  OS << "inline std::vector<SPIRVExtension> getSupportedExtensionInfo(PLATFORM Platform) {\n";
  OS << "  std::vector<SPIRVExtension> SupportedExtensions;\n";
  OS << "  for (const auto& Ext : AllExtensions) {\n";
  OS << "    if (isExtensionSupported(Ext.Name, Platform)) {\n";
  OS << "      SPIRVExtension SupportedExt;\n";
  OS << "      SupportedExt.Name = Ext.Name;\n";
  OS << "      SupportedExt.SpecURL = Ext.SpecURL;\n";
  OS << "      for (const auto& Cap : Ext.Capabilities) {\n";
  OS << "        if (isCapabilitySupported(Cap.Name, Platform)) {\n";
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
enum ActionType { EmitSPIRVDocs, EmitOptionsDocs, EmitSPIRVExtensionSupportHeader };

cl::opt<ActionType> Action(
    cl::desc("Action to perform:"),
    cl::values(clEnumValN(EmitSPIRVDocs, "gen-igcc-spirv-extensions-docs",
                          "Generate IGCCompute supported SPIR-V extensions documentation"),
               clEnumValN(EmitOptionsDocs, "gen-igcc-options-docs",
                          "Generate IGCCompute supported options documentation"),
               clEnumValN(EmitSPIRVExtensionSupportHeader, "gen-igcc-spirv-extension-support-header",
                          "Generate IGCCompute SPIR-V extension support query header (structures + query functions)")));
} // namespace

static bool OptionsAndDocsTblgenMain(raw_ostream &OS, RecordKeeper &Records) {
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
