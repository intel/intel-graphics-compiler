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
def IGFX_CRI            : Platform<"IGFX_CRI">;

// Core Families
def IGFX_GEN12_CORE     : CoreFamily<"IGFX_GEN12_CORE">;
def IGFX_GEN12LP_CORE   : CoreFamily<"IGFX_GEN12LP_CORE">;
def IGFX_XE_HP_CORE     : CoreFamily<"IGFX_XE_HP_CORE">;
def IGFX_XE_HPG_CORE    : CoreFamily<"IGFX_XE_HPG_CORE">;
def IGFX_XE_HPC_CORE    : CoreFamily<"IGFX_XE_HPC_CORE">;
def IGFX_XE2_HPG_CORE   : CoreFamily<"IGFX_XE2_HPG_CORE">;
def IGFX_XE3_CORE       : CoreFamily<"IGFX_XE3_CORE">;
def IGFX_XE3P_CORE      : CoreFamily<"IGFX_XE3P_CORE">;

// Define Platform Groups for common use cases
def CUSTOM_GROUP : PlatformGroup<[IGFX_DG2, IGFX_PVC]>;

// NOTE: The list of extensions below is INCOMPLETE and will be expanded
// incrementally in future commits. Current entries may have provisional or
// incorrect PlatformSupport assignments; they are included now to demonstrate
// intended usage and integration flow.

// --- Extensions supported on all platforms ---

def SPV_EXT_optnone : Extension<
  "SPV_EXT_optnone",
  [ Capability<"OptNoneEXT"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_optnone.html"
>;

// SPV_INTEL_optnone is an old name for the same extension as SPV_EXT_optnone.
// We must keep it for compatibility with SPIRV produced by older frontends.
// By marking it as ExperimentalExtension we avoid advertising it in the list of supported extensions,
// while it is still accepted by SPIRV validator.
def SPV_INTEL_optnone : ExperimentalExtension<
  "SPV_INTEL_optnone",
  [ Capability<"OptNoneINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_optnone.html"
>;

def SPV_INTEL_cache_controls : Extension<
  "SPV_INTEL_cache_controls",
  [ Capability<"CacheControlsINTEL"> ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_cache_controls.asciidoc"
>;

def SPV_INTEL_fp_fast_math_mode : ExperimentalExtension<
  "SPV_INTEL_fp_fast_math_mode",
  [ Capability<"FPFastMathModeINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_fp_fast_math_mode.html"
>;

def SPV_INTEL_sigmoid : ExperimentalExtension<
  "SPV_INTEL_sigmoid",
  [ Capability<"SigmoidINTEL"> ],
  "https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_sigmoid.asciidoc",
  AllPlatformSupport
>;

def SPV_INTEL_function_pointers : ExperimentalExtension<
  "SPV_INTEL_function_pointers",
  [ Capability<"FunctionPointersINTEL">,
    Capability<"IndirectReferencesINTEL"> ],
  "https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_function_pointers.asciidoc"
>;

def SPV_INTEL_global_variable_host_access : Extension<
  "SPV_INTEL_global_variable_host_access",
  [ Capability<"GlobalVariableHostAccessINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_global_variable_host_access.html"
>;

// SPV_INTEL_global_variable_decorations has been split, into SPV_INTEL_global_variable_host_access
// and SPV_INTEL_global_variable_fpga_decorations, while publishing it to Khronos SPIR-V Registry.
// Defining this extension to keep compatibility with old SPIR-V producers. Without it, IGC's SPIR-V Reader
// will reject modules produced by those old frontends. It is intentionally marked as ExperimentalExtension
// to avoid advertising it in the list of supported extensions.
def SPV_INTEL_global_variable_decorations : ExperimentalExtension<
  "SPV_INTEL_global_variable_decorations",
  [],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_global_variable_host_access.html"
>;

def SPV_INTEL_inline_assembly : ExperimentalExtension<
  "SPV_INTEL_inline_assembly",
  [ Capability<"AsmINTEL"> ],
  "https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_inline_assembly.asciidoc"
>;

def SPV_INTEL_subgroups : Extension<
  "SPV_INTEL_subgroups",
  [ Capability<"SubgroupShuffleINTEL">,
    Capability<"SubgroupBufferBlockIOINTEL">,
    Capability<"SubgroupImageBlockIOINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_subgroups.html"
>;

def SPV_INTEL_subgroup_buffer_prefetch : Extension<
  "SPV_INTEL_subgroup_buffer_prefetch",
  [ Capability<"SubgroupBufferPrefetchINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_subgroup_buffer_prefetch.html"
>;

def SPV_INTEL_variable_length_array : ExperimentalExtension<
  "SPV_INTEL_variable_length_array",
  [ Capability<"VariableLengthArrayINTEL">,
    Capability<"UntypedVariableLengthArrayINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_variable_length_array.html"
>;

def SPV_KHR_bit_instructions : Extension<
  "SPV_KHR_bit_instructions",
  [ Capability<"BitInstructions"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_bit_instructions.html"
>;

def SPV_KHR_expect_assume : Extension<
  "SPV_KHR_expect_assume",
  [ Capability<"ExpectAssumeKHR"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_expect_assume.html"
>;

def SPV_KHR_linkonce_odr : Extension<
  "SPV_KHR_linkonce_odr",
  [ Capability<"Linkage"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_linkonce_odr.html"
>;

def SPV_KHR_no_integer_wrap_decoration : Extension<
  "SPV_KHR_no_integer_wrap_decoration",
  [],
  "https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_no_integer_wrap_decoration.html"
>;

def SPV_INTEL_fp_max_error : ExperimentalExtension<
  "SPV_INTEL_fp_max_error",
  [ Capability<"FPMaxErrorINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_fp_max_error.html"
>;

def SPV_INTEL_long_composites : Extension<
  "SPV_INTEL_long_composites",
  [ Capability<"CapabilityLongCompositesINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_long_composites.html"
>;

def SPV_INTEL_split_barrier : Extension<
  "SPV_INTEL_split_barrier",
  [ Capability<"SplitBarrierINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_split_barrier.html"
>;

def SPV_INTEL_unstructured_loop_controls : Extension<
  "SPV_INTEL_unstructured_loop_controls",
  [ Capability<"UnstructuredLoopControlsINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_unstructured_loop_controls.html"
>;

def SPV_KHR_integer_dot_product : Extension<
  "SPV_KHR_integer_dot_product",
  [ Capability<"DotProductKHR">,
    Capability<"DotProductInputAllKHR">,
    Capability<"DotProductInput4x8BitKHR">,
    Capability<"DotProductInput4x8BitPackedKHR"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_integer_dot_product.html"
>;

def SPV_KHR_uniform_group_instructions : ExperimentalExtension<
  "SPV_KHR_uniform_group_instructions",
  [ Capability<"GroupUniformArithmeticKHR"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_uniform_group_instructions.html"
>;

def SPV_KHR_shader_clock : Extension<
  "SPV_KHR_shader_clock",
  [ Capability<"ShaderClockKHR"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_shader_clock.html"
>;

def SPV_INTEL_debug_module : ExperimentalExtension<
  "SPV_INTEL_debug_module",
  [ Capability<"DebugInfoModuleINTEL"> ],
  "https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_debug_module.asciidoc"
>;

def SPV_INTEL_memory_access_aliasing : ExperimentalExtension<
  "SPV_INTEL_memory_access_aliasing",
  [ Capability<"MemoryAccessAliasingINTEL"> ],
  "https://github.com/intel/llvm/pull/3426/files"
>;

def SPV_INTEL_predicated_io : ExperimentalExtension<
  "SPV_INTEL_predicated_io",
  [ Capability<"PredicatedIOINTEL"> ],
  "https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_predicated_io.asciidoc"
>;

// Some SPIR-V producers may emit arbitrary precision integer types, often as a result of LLVM optimizations in frontends.
// IGC provides partial support for this extension to enable processing of such SPIR-V modules.
// Supported functionality includes:
// - Integer types with widths divisible by 8 (e.g., 8, 16, 24, 32, ...) up to a maximum size of 1024 bits
// - The largest supported type is i1024, corresponding to the total width of the maximum supported vector type <16 x i64>
def SPV_INTEL_arbitrary_precision_integers : ExperimentalExtension<
  "SPV_INTEL_arbitrary_precision_integers",
  [ Capability<"ArbitraryPrecisionIntegersINTEL"> ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_arbitrary_precision_integers.html"
>;

// This extension is supported to allow SPIR-V modules containing non-semantic debug info to be compiled successfully.
// (https://github.khronos.org/SPIRV-Registry/nonsemantic/NonSemantic.Shader.DebugInfo.100.html)
def SPV_KHR_non_semantic_info : ExperimentalExtension<
  "SPV_KHR_non_semantic_info",
  [],
  "https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_non_semantic_info.html"
>;

// Scalar IGC compilation path does not support SPV_INTEL_vector_compute extension. However, some
// cassian tests use ext_vector_type to define bigger than 16-elements vector types. Khronos SPIRVWriter
// automatically add SPV_INTEL_vector_compute extension if a module contains such vector types.
// To avoid compilation failure in such cases, we mark this extension as supported on all platforms.
def SPV_INTEL_vector_compute : ExperimentalExtension<
  "SPV_INTEL_vector_compute",
  [ Capability<"VectorComputeINTEL">,
    Capability<"VectorAnyINTEL"> ],
  "https://github.com/intel/llvm/pull/1612"
>;

// ---------------------------------------------

// --- Extensions not supported on all platforms ---

def SPV_INTEL_2d_block_io : Extension<
  "SPV_INTEL_2d_block_io",
  [ Capability<"Subgroup2DBlockIOINTEL">,
    Capability<"Subgroup2DBlockTransformINTEL">,
    Capability<"Subgroup2DBlockTransposeINTEL"> ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_2d_block_io.asciidoc",
  isCoreChildOf<IGFX_XE_HPC_CORE>
>;

def SPV_INTEL_media_block_io : Extension<
  "SPV_INTEL_media_block_io",
  [ Capability<"SubgroupImageMediaBlockIOINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_media_block_io.html",
  AnyOf<[ExactCoreFamily<IGFX_GEN12LP_CORE>, ExactCoreFamily<IGFX_XE_HPG_CORE>]>
>;

def SPV_INTEL_kernel_attributes : ExperimentalExtension<
  "SPV_INTEL_kernel_attributes",
  [ Capability<"KernelAttributesINTEL", AllPlatformSupport>,
    Capability<"FPGAKernelAttributesINTEL", NotSupported>,
    Capability<"FPGAKernelAttributesv2INTEL", NotSupported> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_kernel_attributes.html",
  InheritFromCapabilities
>;

def SPV_INTEL_tensor_float32_conversion : Extension<
  "SPV_INTEL_tensor_float32_conversion",
  [ Capability<"TensorFloat32RoundingINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_tensor_float32_conversion.html",
  isCoreChildOf<IGFX_XE_HPC_CORE>
>;

def SPV_EXT_shader_atomic_float_add : Extension<
  "SPV_EXT_shader_atomic_float_add",
  [ Capability<"AtomicFloat32AddEXT", AllPlatformSupport>,
    Capability<"AtomicFloat64AddEXT", isProductChildOf<IGFX_METEORLAKE>> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_shader_atomic_float_add.html",
  InheritFromCapabilities
>;

def SPV_INTEL_joint_matrix : ExperimentalExtension<
  "SPV_INTEL_joint_matrix",
  [ Capability<"PackedCooperativeMatrixINTEL", isCoreChildOf<IGFX_XE_HPG_CORE>>,
    Capability<"CooperativeMatrixInvocationInstructionsINTEL", isCoreChildOf<IGFX_XE_HPG_CORE>>,
    Capability<"CooperativeMatrixTF32ComponentTypeINTEL", isCoreChildOf<IGFX_XE_HPC_CORE>>,
    Capability<"CooperativeMatrixBFloat16ComponentTypeINTEL", isCoreChildOf<IGFX_XE_HPG_CORE>>,
    Capability<"CooperativeMatrixPrefetchINTEL", isCoreChildOf<IGFX_XE_HPC_CORE>> ],
  "https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_joint_matrix.asciidoc",
  InheritFromCapabilities
>;

def SPV_KHR_cooperative_matrix : ExperimentalExtension<
  "SPV_KHR_cooperative_matrix",
  [ Capability<"CooperativeMatrixKHR"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_cooperative_matrix.html",
  isCoreChildOf<IGFX_XE_HPG_CORE>
>;

def SPV_INTEL_subgroup_matrix_multiply_accumulate : Extension<
  "SPV_INTEL_subgroup_matrix_multiply_accumulate",
  [ Capability<"SubgroupMatrixMultiplyAccumulateINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_subgroup_matrix_multiply_accumulate.html",
  AllOf<[isCoreChildOf<IGFX_XE_HPG_CORE>, Not<ExactPlatform<IGFX_METEORLAKE>>, Not<ExactPlatform<IGFX_ARROWLAKE>>]>
>;

def SPV_INTEL_bindless_images : ExperimentalExtension<
  "SPV_INTEL_bindless_images",
  [ Capability<"BindlessImagesINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_bindless_images.html",
  AllOf<[isCoreChildOf<IGFX_XE_HPG_CORE>, Not<ExactPlatform<IGFX_PVC>>, Not<ExactPlatform<IGFX_CRI>>]>
>;

def SPV_EXT_float8 : ExperimentalExtension<
  "SPV_EXT_float8",
  [ Capability<"Float8EXT">,
    Capability<"Float8CooperativeMatrixEXT"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_float8.html",
  isCoreChildOf<IGFX_XE3P_CORE>
>;

def SPV_EXT_shader_atomic_float16_add : Extension<
  "SPV_EXT_shader_atomic_float16_add",
  [ Capability<"AtomicFloat16AddEXT"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_shader_atomic_float16_add.html",
  isCoreChildOf<IGFX_XE3P_CORE>
>;

def SPV_EXT_shader_atomic_float_min_max : Extension<
  "SPV_EXT_shader_atomic_float_min_max",
  [ Capability<"AtomicFloat16MinMaxEXT", isCoreChildOf<IGFX_XE3P_CORE>>,
    Capability<"AtomicFloat32MinMaxEXT", AllPlatformSupport>,
    Capability<"AtomicFloat64MinMaxEXT", isProductChildOf<IGFX_METEORLAKE>> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_shader_atomic_float_min_max.html",
  InheritFromCapabilities
>;

def SPV_INTEL_bfloat16_arithmetic : ExperimentalExtension<
  "SPV_INTEL_bfloat16_arithmetic",
  [ Capability<"BFloat16ArithmeticINTEL"> ],
  "https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_bfloat16_arithmetic.asciidoc",
  // isCoreChildOf<IGFX_XE3P_CORE>
  // This extension has been mainly implemented for XE3P, but there are existing LIT tests for some functionality from
  // this extension for PVC and DG2. We may consider supporting those platforms fully in the future. Since the support
  // for this extension is still experimental, we define support for broader set of platforms for now to avoid fails
  // on existing PVC and DG2 tests.
  AllOf<[isCoreChildOf<IGFX_XE_HPG_CORE>, Not<ExactPlatform<IGFX_METEORLAKE>>]>
>;

def SPV_INTEL_bfloat16_conversion : Extension<
  "SPV_INTEL_bfloat16_conversion",
  [ Capability<"BFloat16ConversionINTEL"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_bfloat16_conversion.html",
  AllOf<[isCoreChildOf<IGFX_XE_HPG_CORE>, Not<ExactPlatform<IGFX_METEORLAKE>>]>
>;

def SPV_INTEL_int4 : ExperimentalExtension<
  "SPV_INTEL_int4",
  [ Capability<"Int4TypeINTEL", isCoreChildOf<IGFX_XE3P_CORE>>,
    Capability<"Int4CooperativeMatrixINTEL", NotSupported> ],
  "https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_int4.asciidoc",
  InheritFromCapabilities
>;

def SPV_INTEL_fp_conversions : ExperimentalExtension<
  "SPV_INTEL_fp_conversions",
  [ Capability<"FloatConversionsINTEL"> ],
  "https://github.com/intel/llvm/pull/20467",
  isCoreChildOf<IGFX_XE3P_CORE>
>;

def SPV_INTEL_float4 : ExperimentalExtension<
  "SPV_INTEL_float4",
  [ Capability<"Float4TypeINTEL">,
    Capability<"Float4CooperativeMatrixINTEL"> ],
  "https://github.com/intel/llvm/pull/20467",
  isCoreChildOf<IGFX_XE3P_CORE>
>;

def SPV_INTEL_16bit_atomics : ExperimentalExtension<
  "SPV_INTEL_16bit_atomics",
  [ Capability<"AtomicInt16CompareExchangeINTEL", isCoreChildOf<IGFX_XE3P_CORE>>,
    Capability<"Int16AtomicsINTEL", NotSupported>,
    Capability<"AtomicBFloat16LoadStoreINTEL", isCoreChildOf<IGFX_XE3P_CORE>>,
    Capability<"AtomicBFloat16AddINTEL", isCoreChildOf<IGFX_XE3P_CORE>>,
    Capability<"AtomicBFloat16MinMaxINTEL", isCoreChildOf<IGFX_XE3P_CORE>> ],
  "https://github.com/intel/llvm/pull/20009",
  InheritFromCapabilities
>;

// Pure support only for isCoreChildOf<IGFX_XE3P_CORE>>, other platforms have partial native support which requires emulation
def SPV_KHR_bfloat16 : ExperimentalExtension<
  "SPV_KHR_bfloat16",
  [ Capability<"BFloat16TypeKHR">,
    Capability<"BFloat16DotProductKHR">,
    Capability<"BFloat16CooperativeMatrixKHR"> ],
  "https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_bfloat16.html",
  // isCoreChildOf<IGFX_XE3P_CORE>
  // This extension has been mainly implemented for XE3P, but there are existing LIT tests for some functionality from
  // this extension for PVC and DG2. We may consider supporting those platforms fully in the future. Since the support
  // for this extension is still experimental, we define support for broader set of platforms for now to avoid fails
  // on existing PVC and DG2 tests.
  AllOf<[isCoreChildOf<IGFX_XE_HPG_CORE>, Not<ExactPlatform<IGFX_METEORLAKE>>]>
>;

def SPV_INTEL_subgroup_scaled_matrix_multiply_accumulate : ExperimentalExtension<
  "SPV_INTEL_subgroup_scaled_matrix_multiply_accumulate",
  [ Capability<"Subgroup​Scaled​Matrix​Multiply​Accumulate​INTEL"> ],
  "https://github.com/intel/llvm/pull/20656",
  isCoreChildOf<IGFX_XE3P_CORE>
>;

def SPV_INTEL_device_barrier : ExperimentalExtension<
  "SPV_INTEL_device_barrier",
  [ Capability<"DeviceBarrierINTEL"> ],
  "https://github.com/intel/llvm/pull/12092"
>;

// ----------------------------------------------
