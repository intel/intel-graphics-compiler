/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
//===- SPIRVExtensions.td - SPIR-V Extensions supported by igc-compute ----*- tablegen -*-===//
//
// This file declares a set of SPIR-V extensions supported by igc-compute.
//
//===-------------------------------------------------------------------------------------===//

// Note! After editing this file, please run the build locally, to generate the documentation.

// SPIRVExtension class accepts:
//   1) The extension name
//   2) A list of supported capabilities in this extension
//   3) Specification URL
class SPIRVExtension<
    string ExtensionName,
    list<string> Capabilities,
    string SpecURL
> {
  string ExtName = ExtensionName;
  list<string> ExtCapabilities = Capabilities;
  string ExtSpecURL = SpecURL;
}

// Note: the list is incomplete. TODO: extend the list with all supported extensions.

def SPV_INTEL_subgroups : SPIRVExtension<
  "SPV_INTEL_subgroups",
  [ "SubgroupShuffleINTEL",
    "SubgroupBufferBlockIOINTEL",
    "SubgroupImageBlockIOINTEL" ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_subgroups.asciidoc"
>;

def SPV_INTEL_bfloat16_conversion : SPIRVExtension<
  "SPV_INTEL_bfloat16_conversion",
  [ "BFloat16ConversionINTEL" ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_bfloat16_conversion.asciidoc"
>;

def SPV_INTEL_2d_block_io : SPIRVExtension<
  "SPV_INTEL_2d_block_io",
  [ "Subgroup2DBlockIOINTEL",
    "Subgroup2DBlockTransformINTEL",
    "Subgroup2DBlockTransposeINTEL" ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_2d_block_io.asciidoc"
>;

def SPV_INTEL_cache_controls : SPIRVExtension<
  "SPV_INTEL_cache_controls",
  [ "CacheControlsINTEL" ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_cache_controls.asciidoc"
>;

def SPV_INTEL_fp_fast_math_mode : SPIRVExtension<
  "SPV_INTEL_fp_fast_math_mode",
  [ "FPFastMathModeINTEL" ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_fp_fast_math_mode.asciidoc"
>;

