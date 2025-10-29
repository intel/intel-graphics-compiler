/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ocl_igc_interface/impl/igc_options_and_capabilities_impl.h"
#include "ocl_igc_interface/igc_options_and_capabilities.h"

#include "cif/helpers/error.h"
#include "cif/macros/enable.h"

#if __has_include("IGCCSPIRVExtensionsYaml.inc")
#include "IGCCSPIRVExtensionsYaml.inc"
#else
static const char SPIRVExtensionsYAML[] = "";
#endif

#include "Options/include/igc/Options/Options.h"

#include <llvm/Support/raw_ostream.h>

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

  OutSupportedSPIRVExtensionsYAML->PushBackRawBytes(SPIRVExtensionsYAML, sizeof(SPIRVExtensionsYAML));
}

} // namespace IGC

#include "cif/macros/disable.h"
