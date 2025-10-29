/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/builtins/memory/buffer/buffer.h"
#include "cif/common/cif.h"
#include "cif/common/id.h"

#include "cif/macros/enable.h"
#include "OCLAPI/oclapi.h"

// Interface : IGC_OP_CA
//             IGC options and capabilities
// Interface for defining compiler options and capabilities

namespace IGC {

CIF_DECLARE_INTERFACE(IgcOptionsAndCapabilities, "IGC_OP_CA")

CIF_DEFINE_INTERFACE_VER(IgcOptionsAndCapabilities, 1) {
  CIF_INHERIT_CONSTRUCTOR();

  // return buffer format: string with the help message.
  virtual void GetCompilerAPIOptionsHelp(CIF::Builtins::BufferSimple *
                                         outAPIOptions) const;

  // return buffer format: string with the help message.
  virtual void GetCompilerInternalOptionsHelp(CIF::Builtins::BufferSimple *
                                              outInternalOptions) const;

  // clang-format off
  // return buffer format: string in YAML format described as:
  //
  // spirv_extensions:
  //   - name: SPV_INTEL_subgroups
  //     url: "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_subgroups.asciidoc"
  //     supported_capabilities:
  //       - SubgroupShuffleINTEL
  //       - SubgroupBufferBlockIOINTEL
  //       - SubgroupImageBlockIOINTEL
  //   - name: SPV_INTEL_cache_controls
  //     url: "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_cache_controls.asciidoc"
  //     supported_capabilities:
  //       - CacheControlsINTEL
  //
  // clang-format on
  OCL_API_CALL virtual void GetCompilerSupportedSPIRVExtensionsYAML(CIF::Builtins::BufferSimple *
                                                                    outSupportedSPIRVExtensionsYAML) const;
};

CIF_GENERATE_VERSIONS_LIST(IgcOptionsAndCapabilities);
CIF_MARK_LATEST_VERSION(IgcOptionsAndCapabilitiesLatest, IgcOptionsAndCapabilities);

// Example for transition periods.
// using IgcOptionsAndCapabilitiesTagOCL =
//     IgcOptionsAndCapabilities<1>;

using IgcOptionsAndCapabilitiesTagOCL = IgcOptionsAndCapabilitiesLatest;
// Note : can tag with different version for
//         transition periods
} // namespace IGC

#include "cif/macros/disable.h"
