/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Include common definitions
include "SPIRVExtensions_Common.td"

//===----------------------------------------------------------------------===//
//                     SPIR-V Extension Definitions
//===----------------------------------------------------------------------===//
//
// For complete documentation about class structures, platform support types,
// extension modes, validation rules, and usage examples, see SPIRVExtensions_Common.td
//
//===----------------------------------------------------------------------===//

// GPU Platforms
def IGFX_TIGERLAKE_LP   : Platform<"IGFX_TIGERLAKE_LP">;
def IGFX_ROCKETLAKE     : Platform<"IGFX_ROCKETLAKE">;
def IGFX_ALDERLAKE_S    : Platform<"IGFX_ALDERLAKE_S">;
def IGFX_ALDERLAKE_P    : Platform<"IGFX_ALDERLAKE_P">;
def IGFX_ALDERLAKE_N    : Platform<"IGFX_ALDERLAKE_N">;
def IGFX_DG1            : Platform<"IGFX_DG1">;
def IGFX_XE_HP_SDV      : Platform<"IGFX_XE_HP_SDV">;
def IGFX_DG2            : Platform<"IGFX_DG2">;
def IGFX_PVC            : Platform<"IGFX_PVC">;
def IGFX_METEORLAKE     : Platform<"IGFX_METEORLAKE">;
def IGFX_ARROWLAKE      : Platform<"IGFX_ARROWLAKE">;
def IGFX_BMG            : Platform<"IGFX_BMG">;
def IGFX_LUNARLAKE      : Platform<"IGFX_LUNARLAKE">;
def IGFX_PTL            : Platform<"IGFX_PTL">;
def IGFX_NVL_XE3G       : Platform<"IGFX_NVL_XE3G">;

// Core Families
def IGFX_GEN12_CORE     : CoreFamily<"IGFX_GEN12_CORE">;
def IGFX_GEN12LP_CORE   : CoreFamily<"IGFX_GEN12LP_CORE">;
def IGFX_XE_HP_CORE     : CoreFamily<"IGFX_XE_HP_CORE">;
def IGFX_XE_HPG_CORE    : CoreFamily<"IGFX_XE_HPG_CORE">;
def IGFX_XE_HPC_CORE    : CoreFamily<"IGFX_XE_HPC_CORE">;
def IGFX_XE2_HPG_CORE   : CoreFamily<"IGFX_XE2_HPG_CORE">;
def IGFX_XE3_CORE       : CoreFamily<"IGFX_XE3_CORE">;

// Define Platform Groups for common use cases
def CUSTOM_GROUP : PlatformGroup<[IGFX_DG2, IGFX_PVC]>;

// NOTE: The list of extensions below is INCOMPLETE and will be expanded
// incrementally in future commits. Current entries may have provisional or
// incorrect PlatformSupport assignments; they are included now to demonstrate
// intended usage and integration flow.

def SPV_INTEL_subgroups : Extension<
  "SPV_INTEL_subgroups",
  [ Capability<"SubgroupShuffleINTEL">,
    Capability<"SubgroupBufferBlockIOINTEL">,
    Capability<"SubgroupImageBlockIOINTEL"> ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_subgroups.asciidoc"
>;

def SPV_INTEL_bfloat16_conversion : Extension<
  "SPV_INTEL_bfloat16_conversion",
  [ Capability<"BFloat16ConversionINTEL"> ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_bfloat16_conversion.asciidoc"
>;

def SPV_INTEL_2d_block_io : Extension<
  "SPV_INTEL_2d_block_io",
  [ Capability<"Subgroup2DBlockIOINTEL">,
    Capability<"Subgroup2DBlockTransformINTEL">,
    Capability<"Subgroup2DBlockTransposeINTEL"> ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_2d_block_io.asciidoc"
>;

def SPV_INTEL_cache_controls : Extension<
  "SPV_INTEL_cache_controls",
  [ Capability<"CacheControlsINTEL"> ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_cache_controls.asciidoc"
>;

def SPV_INTEL_fp_fast_math_mode : Extension<
  "SPV_INTEL_fp_fast_math_mode",
  [ Capability<"FPFastMathModeINTEL"> ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_fp_fast_math_mode.asciidoc"
>;
