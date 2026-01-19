/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ocl_igc_interface/impl/igc_options_and_capabilities_impl.h"
#include "ocl_igc_interface/igc_options_and_capabilities.h"

#include "ocl_igc_interface/platform.h"

#include "cif/helpers/error.h"
#include "cif/macros/enable.h"

#include "SPIRVExtensionsSupport.h"

#include "Options/include/igc/Options/Options.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/YAMLTraits.h>

// YAML serialization support
LLVM_YAML_IS_SEQUENCE_VECTOR(IGC::SPIRVExtensionsSupport::SPIRVExtension)
LLVM_YAML_IS_SEQUENCE_VECTOR(IGC::SPIRVExtensionsSupport::SPIRVCapability)

namespace llvm {
namespace yaml {

template <> struct MappingTraits<IGC::SPIRVExtensionsSupport::SPIRVCapability> {
  static void mapping(IO &Io, IGC::SPIRVExtensionsSupport::SPIRVCapability &Cap) { Io.mapRequired("name", Cap.Name); }
};

template <> struct MappingTraits<IGC::SPIRVExtensionsSupport::SPIRVExtension> {
  static void mapping(IO &io, IGC::SPIRVExtensionsSupport::SPIRVExtension &Ext) {
    io.mapRequired("name", Ext.Name);
    io.mapRequired("spec_url", Ext.SpecURL);
    io.mapRequired("supported_capabilities", Ext.Capabilities);
  }
};

} // namespace yaml
} // namespace llvm

namespace {
// Helper function to fill BufferSimple with help string for given OptTable.
void fillHelpString(const llvm::opt::OptTable &OptTable, const char *Title, const char *Usage,
                    CIF::Builtins::BufferSimple *OutBuffer) {
  assert(OutBuffer);
  std::string Str;
  llvm::raw_string_ostream Outstr(Str);
  OptTable.printHelp(Outstr, Usage, Title, false, true);
  OutBuffer->PushBackRawBytes(Str.c_str(), Str.size());
}

// Helper function to generate YAML from extension info
void generateSPIRVExtensionsYAML(const std::vector<IGC::SPIRVExtensionsSupport::SPIRVExtension> &extensions,
                                 CIF::Builtins::BufferSimple *OutBuffer) {
  std::string YamlString;
  llvm::raw_string_ostream yamlStream(YamlString);

  llvm::yaml::Output yout(yamlStream);
  yout << const_cast<std::vector<IGC::SPIRVExtensionsSupport::SPIRVExtension> &>(extensions);

  OutBuffer->PushBackRawBytes(YamlString.c_str(), YamlString.size());
}
} // namespace
namespace IGC {

void CIF_GET_INTERFACE_CLASS(IgcOptionsAndCapabilities,
                             1)::GetCompilerAPIOptionsHelp(CIF::Builtins::BufferSimple *outAPIOptions) const {
  assert(outAPIOptions && outAPIOptions->GetSizeRaw() == 0 && "GetCompilerAPIOptionsHelp expects empty buffer");

  const llvm::opt::OptTable &ApiOptionsTable = IGC::getApiOptTable();
  fillHelpString(ApiOptionsTable, "IGC Options", nullptr, outAPIOptions);
}

void CIF_GET_INTERFACE_CLASS(IgcOptionsAndCapabilities,
                             1)::GetCompilerInternalOptionsHelp(CIF::Builtins::BufferSimple *outInternalOptions) const {
  assert(outInternalOptions && outInternalOptions->GetSizeRaw() == 0 &&
         "GetCompilerInternalOptionsHelp expects empty buffer");

  const llvm::opt::OptTable &InternalOptionsTable = IGC::getInternalOptTable();
  fillHelpString(InternalOptionsTable, "IGC Internal Options", nullptr, outInternalOptions);
}

void CIF_GET_INTERFACE_CLASS(IgcOptionsAndCapabilities, 1)::GetCompilerSupportedSPIRVExtensionsYAML(
    CIF::Builtins::BufferSimple *OutSupportedSPIRVExtensionsYAML) const {
  assert(OutSupportedSPIRVExtensionsYAML && OutSupportedSPIRVExtensionsYAML->GetSizeRaw() == 0 &&
         "GetCompilerSupportedSPIRVExtensionsYAML expects empty buffer");

  auto *PlatformPtr = this->GetImpl()->GetPlatform();
  auto *PlatformImpl = PlatformPtr->GetImpl();

  std::vector<IGC::SPIRVExtensionsSupport::SPIRVExtension> supportedExtensions =
      IGC::SPIRVExtensionsSupport::getSupportedExtensionInfo(PlatformImpl->p);
  generateSPIRVExtensionsYAML(supportedExtensions, OutSupportedSPIRVExtensionsYAML);
}

} // namespace IGC

#include "cif/macros/disable.h"
