/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
//===----------------------------------------------------------------------===//
//
/// \file
/// This tablegen backend emits headers for YAML interfaces related to
/// igc-compute options and capabilities, like supported SPIR-V extensions.
///
/// It is also capable of emitting documentation for the supported SPIR-V
/// extensions and compilation options
//===----------------------------------------------------------------------===//
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Error.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

using namespace llvm;
using namespace std;

#define DEBUG_TYPE "IGCCOptionsAndCapabilitiesEmitter"

namespace {
struct SPIRVExtensionInfo {
  StringRef ExtName;
  StringRef ExtSpecURL;
  std::vector<StringRef> ExtCapabilities;
};

struct SPIRVExtensionsList {
  std::vector<SPIRVExtensionInfo> Extensions;
};
} // namespace

LLVM_YAML_IS_SEQUENCE_VECTOR(SPIRVExtensionInfo)
namespace llvm {
namespace yaml {

template <> struct MappingTraits<SPIRVExtensionInfo> {
  static void mapping(IO &IO, SPIRVExtensionInfo &info) {
    IO.mapRequired("name", info.ExtName);
    IO.mapRequired("spec_url", info.ExtSpecURL);
    IO.mapRequired("supported_capabilities", info.ExtCapabilities);
  }
};

template <> struct MappingTraits<SPIRVExtensionsList> {
  static void mapping(IO &IO, SPIRVExtensionsList &ExtList) { IO.mapRequired("spirv_extensions", ExtList.Extensions); }
};
} // namespace yaml
} // namespace llvm

namespace {

class IGCCOptionsAndCapabilitiesEmitter {
  const RecordKeeper &Records;

public:
  IGCCOptionsAndCapabilitiesEmitter(const RecordKeeper &R) : Records(R) {}
  void emitSPIRVYaml(raw_ostream &OS);
  void emitSPIRVDocs(raw_ostream &OS);
  void emitOptionsDocs(raw_ostream &OS);

private:
  void getSPIRVExtensionList(SPIRVExtensionsList &ExtList);
};

} // end anonymous namespace

void IGCCOptionsAndCapabilitiesEmitter::getSPIRVExtensionList(SPIRVExtensionsList &ExtList) {
  auto AllExtensions = Records.getAllDerivedDefinitions("SPIRVExtension");
  for (const Record *Ext : AllExtensions) {
    StringRef ExtName = Ext->getValueAsString("ExtName");
    StringRef ExtSpecURL = Ext->getValueAsString("ExtSpecURL");
    auto Capabilities = Ext->getValueAsListOfStrings("ExtCapabilities");
    ExtList.Extensions.push_back({ExtName, ExtSpecURL, Capabilities});
  }
}

void IGCCOptionsAndCapabilitiesEmitter::emitSPIRVYaml(raw_ostream &OS) {
  OS << "#ifndef IGCC_LIB_INTERFACE_SPIRV_EXTENSIONS_H\n";
  OS << "#define IGCC_LIB_INTERFACE_SPIRV_EXTENSIONS_H\n";
  OS << "static const char SPIRVExtensionsYAML[] = R\"(\n";
  yaml::Output Yout(OS);
  SPIRVExtensionsList ExtList;
  getSPIRVExtensionList(ExtList);
  Yout << ExtList;
  OS << ")\";\n";
  OS << "#endif\n";
}

void IGCCOptionsAndCapabilitiesEmitter::emitSPIRVDocs(raw_ostream &OS) {
  SPIRVExtensionsList ExtList;
  getSPIRVExtensionList(ExtList);
  OS << "# Supported SPIR-V Extensions\n";
  for (const auto &Ext : ExtList.Extensions) {
    OS << "### " << Ext.ExtName << "\n";
    OS << "  - Spec URL: " << Ext.ExtSpecURL << "\n";
    OS << "  - Supported Capabilities:\n";
    for (const auto &Cap : Ext.ExtCapabilities) {
      OS << "    - " << Cap << "\n";
    }
  }
}

void IGCCOptionsAndCapabilitiesEmitter::emitOptionsDocs(raw_ostream &OS) {
  // To be implemented
}

static void emitSPIRVYaml(const RecordKeeper &Records, raw_ostream &OS) {
  IGCCOptionsAndCapabilitiesEmitter(Records).emitSPIRVYaml(OS);
}

static void emitSPIRVDocs(const RecordKeeper &Records, raw_ostream &OS) {
  IGCCOptionsAndCapabilitiesEmitter(Records).emitSPIRVDocs(OS);
}

static void emitOptionsDocs(const RecordKeeper &Records, raw_ostream &OS) {
  IGCCOptionsAndCapabilitiesEmitter(Records).emitOptionsDocs(OS);
}

namespace {
enum ActionType { EmitSPIRVYaml, EmitSPIRVDocs, EmitOptionsDocs };

cl::opt<ActionType> Action(cl::desc("Action to perform:"),
                           cl::values(clEnumValN(EmitSPIRVYaml, "gen-igcc-spirv-extensions-yaml",
                                                 "Generate IGCCompute supported SPIR-V extensions YAML"),
                                      clEnumValN(EmitSPIRVDocs, "gen-igcc-spirv-extensions-docs",
                                                 "Generate IGCCompute supported SPIR-V extensions documentation"),
                                      clEnumValN(EmitOptionsDocs, "gen-igcc-options-docs",
                                                 "Generate IGCCompute supported options documentation")));
} // namespace

static bool OptionsAndDocsTblgenMain(raw_ostream &OS, RecordKeeper &Records) {
  switch (Action) {
  case EmitSPIRVYaml:
    emitSPIRVYaml(Records, OS);
    break;
  case EmitSPIRVDocs:
    emitSPIRVDocs(Records, OS);
    break;
  case EmitOptionsDocs:
    emitOptionsDocs(Records, OS);
    break;
  }

  return false;
}

int main(int argc, char **argv) {
  InitLLVM y(argc, argv);
  cl::ParseCommandLineOptions(argc, argv);
  return TableGenMain(argv[0], &OptionsAndDocsTblgenMain);
}
