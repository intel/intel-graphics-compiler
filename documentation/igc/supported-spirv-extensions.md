# Supported SPIR-V Extensions

This document lists all SPIR-V extensions supported by IGC and their platform requirements.

## SPV_EXT_float8

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_float8.html

> **Experimentally supported on**: IGFX_XE3P_CORE and newer

**Capabilities**:

- **Float8EXT**
- **Float8CooperativeMatrixEXT**

---

## SPV_EXT_optnone

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_optnone.html

> **Supported on**: All platforms

**Capabilities**:

- **OptNoneEXT**

---

## SPV_EXT_shader_atomic_float16_add

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_shader_atomic_float16_add.html

> **Supported on**: IGFX_XE3P_CORE and newer

**Capabilities**:

- **AtomicFloat16AddEXT**

---

## SPV_EXT_shader_atomic_float_add

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_shader_atomic_float_add.html

**Capabilities**:

- **AtomicFloat32AddEXT**
  > **Supported On**: All platforms
- **AtomicFloat64AddEXT**
  > **Supported On**: IGFX_METEORLAKE and newer

---

## SPV_EXT_shader_atomic_float_min_max

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_shader_atomic_float_min_max.html

**Capabilities**:

- **AtomicFloat16MinMaxEXT**
  > **Supported On**: IGFX_XE3P_CORE and newer
- **AtomicFloat32MinMaxEXT**
  > **Supported On**: All platforms
- **AtomicFloat64MinMaxEXT**
  > **Supported On**: IGFX_METEORLAKE and newer

---

## SPV_INTEL_16bit_atomics

**Specification**: https://github.com/intel/llvm/pull/20009

**Capabilities**:

- **AtomicInt16CompareExchangeINTEL**
  > **Experimentally supported on**: IGFX_XE3P_CORE and newer
- **Int16AtomicsINTEL**
  > **Experimentally supported on**: Not supported
- **AtomicBFloat16LoadStoreINTEL**
  > **Experimentally supported on**: IGFX_XE3P_CORE and newer
- **AtomicBFloat16AddINTEL**
  > **Experimentally supported on**: IGFX_XE3P_CORE and newer
- **AtomicBFloat16MinMaxINTEL**
  > **Experimentally supported on**: IGFX_XE3P_CORE and newer

---

## SPV_INTEL_2d_block_io

**Specification**: https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_2d_block_io.asciidoc

> **Supported on**: IGFX_XE_HPC_CORE and newer

**Capabilities**:

- **Subgroup2DBlockIOINTEL**
- **Subgroup2DBlockTransformINTEL**
- **Subgroup2DBlockTransposeINTEL**

---

## SPV_INTEL_arbitrary_precision_integers

**Specification**: https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_arbitrary_precision_integers.html

> **Experimentally supported on**: All platforms

**Capabilities**:

- **ArbitraryPrecisionIntegersINTEL**

---

## SPV_INTEL_bfloat16_arithmetic

**Specification**: https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_bfloat16_arithmetic.asciidoc

> **Experimentally supported on**: IGFX_XE_HPG_CORE and newer (excluding IGFX_METEORLAKE)

**Capabilities**:

- **BFloat16ArithmeticINTEL**

---

## SPV_INTEL_bfloat16_conversion

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_bfloat16_conversion.html

> **Supported on**: IGFX_XE_HPG_CORE and newer (excluding IGFX_METEORLAKE)

**Capabilities**:

- **BFloat16ConversionINTEL**

---

## SPV_INTEL_bindless_images

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_bindless_images.html

> **Experimentally supported on**: IGFX_XE_HPG_CORE and newer (excluding IGFX_PVC, IGFX_CRI)

**Capabilities**:

- **BindlessImagesINTEL**

---

## SPV_INTEL_cache_controls

**Specification**: https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_cache_controls.asciidoc

> **Supported on**: All platforms

**Capabilities**:

- **CacheControlsINTEL**

---

## SPV_INTEL_debug_module

**Specification**: https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_debug_module.asciidoc

> **Experimentally supported on**: All platforms

**Capabilities**:

- **DebugInfoModuleINTEL**

---

## SPV_INTEL_device_barrier

**Specification**: https://github.com/intel/llvm/pull/12092

> **Experimentally supported on**: All platforms

**Capabilities**:

- **DeviceBarrierINTEL**

---

## SPV_INTEL_float4

**Specification**: https://github.com/intel/llvm/pull/20467

> **Experimentally supported on**: IGFX_XE3P_CORE and newer

**Capabilities**:

- **Float4TypeINTEL**
- **Float4CooperativeMatrixINTEL**

---

## SPV_INTEL_fp_conversions

**Specification**: https://github.com/intel/llvm/pull/20467

> **Experimentally supported on**: IGFX_XE3P_CORE and newer

**Capabilities**:

- **FloatConversionsINTEL**

---

## SPV_INTEL_fp_fast_math_mode

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_fp_fast_math_mode.html

> **Experimentally supported on**: All platforms

**Capabilities**:

- **FPFastMathModeINTEL**

---

## SPV_INTEL_fp_max_error

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_fp_max_error.html

> **Experimentally supported on**: All platforms

**Capabilities**:

- **FPMaxErrorINTEL**

---

## SPV_INTEL_function_pointers

**Specification**: https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_function_pointers.asciidoc

> **Experimentally supported on**: All platforms

**Capabilities**:

- **FunctionPointersINTEL**
- **IndirectReferencesINTEL**

---

## SPV_INTEL_global_variable_decorations

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_global_variable_host_access.html

> **Experimentally supported on**: All platforms

**Capabilities**:

- No capabilities defined

---

## SPV_INTEL_global_variable_host_access

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_global_variable_host_access.html

> **Supported on**: All platforms

**Capabilities**:

- **GlobalVariableHostAccessINTEL**

---

## SPV_INTEL_inline_assembly

**Specification**: https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_inline_assembly.asciidoc

> **Experimentally supported on**: All platforms

**Capabilities**:

- **AsmINTEL**

---

## SPV_INTEL_int4

**Specification**: https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_int4.asciidoc

**Capabilities**:

- **Int4TypeINTEL**
  > **Experimentally supported on**: IGFX_XE3P_CORE and newer
- **Int4CooperativeMatrixINTEL**
  > **Experimentally supported on**: Not supported

---

## SPV_INTEL_joint_matrix

**Specification**: https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_joint_matrix.asciidoc

**Capabilities**:

- **PackedCooperativeMatrixINTEL**
  > **Experimentally supported on**: IGFX_XE_HPG_CORE and newer
- **CooperativeMatrixInvocationInstructionsINTEL**
  > **Experimentally supported on**: IGFX_XE_HPG_CORE and newer
- **CooperativeMatrixTF32ComponentTypeINTEL**
  > **Experimentally supported on**: IGFX_XE_HPC_CORE and newer
- **CooperativeMatrixBFloat16ComponentTypeINTEL**
  > **Experimentally supported on**: IGFX_XE_HPG_CORE and newer
- **CooperativeMatrixPrefetchINTEL**
  > **Experimentally supported on**: IGFX_XE_HPC_CORE and newer

---

## SPV_INTEL_kernel_attributes

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_kernel_attributes.html

**Capabilities**:

- **KernelAttributesINTEL**
  > **Experimentally supported on**: All platforms
- **FPGAKernelAttributesINTEL**
  > **Experimentally supported on**: Not supported
- **FPGAKernelAttributesv2INTEL**
  > **Experimentally supported on**: Not supported

---

## SPV_INTEL_long_composites

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_long_composites.html

> **Supported on**: All platforms

**Capabilities**:

- **CapabilityLongCompositesINTEL**

---

## SPV_INTEL_media_block_io

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_media_block_io.html

> **Supported on**: IGFX_GEN12LP_CORE, IGFX_XE_HPG_CORE

**Capabilities**:

- **SubgroupImageMediaBlockIOINTEL**

---

## SPV_INTEL_memory_access_aliasing

**Specification**: https://github.com/intel/llvm/pull/3426/files

> **Experimentally supported on**: All platforms

**Capabilities**:

- **MemoryAccessAliasingINTEL**

---

## SPV_INTEL_optnone

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_optnone.html

> **Experimentally supported on**: All platforms

**Capabilities**:

- **OptNoneINTEL**

---

## SPV_INTEL_predicated_io

**Specification**: https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_predicated_io.asciidoc

> **Experimentally supported on**: All platforms

**Capabilities**:

- **PredicatedIOINTEL**

---

## SPV_INTEL_sigmoid

**Specification**: https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_sigmoid.asciidoc

> **Experimentally supported on**: All platforms

**Capabilities**:

- **SigmoidINTEL**

---

## SPV_INTEL_split_barrier

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_split_barrier.html

> **Supported on**: All platforms

**Capabilities**:

- **SplitBarrierINTEL**

---

## SPV_INTEL_subgroup_buffer_prefetch

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_subgroup_buffer_prefetch.html

> **Supported on**: All platforms

**Capabilities**:

- **SubgroupBufferPrefetchINTEL**

---

## SPV_INTEL_subgroup_matrix_multiply_accumulate

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_subgroup_matrix_multiply_accumulate.html

> **Supported on**: IGFX_XE_HPG_CORE and newer (excluding IGFX_METEORLAKE, IGFX_ARROWLAKE)

**Capabilities**:

- **SubgroupMatrixMultiplyAccumulateINTEL**

---

## SPV_INTEL_subgroup_scaled_matrix_multiply_accumulate

**Specification**: https://github.com/intel/llvm/pull/20656

> **Experimentally supported on**: IGFX_XE3P_CORE and newer

**Capabilities**:

- **Subgroup​Scaled​Matrix​Multiply​Accumulate​INTEL**

---

## SPV_INTEL_subgroups

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_subgroups.html

> **Supported on**: All platforms

**Capabilities**:

- **SubgroupShuffleINTEL**
- **SubgroupBufferBlockIOINTEL**
- **SubgroupImageBlockIOINTEL**

---

## SPV_INTEL_tensor_float32_conversion

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_tensor_float32_conversion.html

> **Supported on**: IGFX_XE_HPC_CORE and newer

**Capabilities**:

- **TensorFloat32RoundingINTEL**

---

## SPV_INTEL_unstructured_loop_controls

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_unstructured_loop_controls.html

> **Supported on**: All platforms

**Capabilities**:

- **UnstructuredLoopControlsINTEL**

---

## SPV_INTEL_variable_length_array

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_variable_length_array.html

> **Experimentally supported on**: All platforms

**Capabilities**:

- **VariableLengthArrayINTEL**
- **UntypedVariableLengthArrayINTEL**

---

## SPV_INTEL_vector_compute

**Specification**: https://github.com/intel/llvm/pull/1612

> **Experimentally supported on**: All platforms

**Capabilities**:

- **VectorComputeINTEL**
- **VectorAnyINTEL**

---

## SPV_KHR_bfloat16

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_bfloat16.html

> **Experimentally supported on**: IGFX_XE_HPG_CORE and newer (excluding IGFX_METEORLAKE)

**Capabilities**:

- **BFloat16TypeKHR**
- **BFloat16DotProductKHR**
- **BFloat16CooperativeMatrixKHR**

---

## SPV_KHR_bit_instructions

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_bit_instructions.html

> **Supported on**: All platforms

**Capabilities**:

- **BitInstructions**

---

## SPV_KHR_cooperative_matrix

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_cooperative_matrix.html

> **Experimentally supported on**: IGFX_XE_HPG_CORE and newer

**Capabilities**:

- **CooperativeMatrixKHR**

---

## SPV_KHR_expect_assume

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_expect_assume.html

> **Supported on**: All platforms

**Capabilities**:

- **ExpectAssumeKHR**

---

## SPV_KHR_integer_dot_product

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_integer_dot_product.html

> **Supported on**: All platforms

**Capabilities**:

- **DotProductKHR**
- **DotProductInputAllKHR**
- **DotProductInput4x8BitKHR**
- **DotProductInput4x8BitPackedKHR**

---

## SPV_KHR_linkonce_odr

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_linkonce_odr.html

> **Supported on**: All platforms

**Capabilities**:

- **Linkage**

---

## SPV_KHR_no_integer_wrap_decoration

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_no_integer_wrap_decoration.html

> **Supported on**: All platforms

**Capabilities**:

- No capabilities defined

---

## SPV_KHR_non_semantic_info

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_non_semantic_info.html

> **Experimentally supported on**: All platforms

**Capabilities**:

- No capabilities defined

---

## SPV_KHR_shader_clock

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_shader_clock.html

> **Supported on**: All platforms

**Capabilities**:

- **ShaderClockKHR**

---

## SPV_KHR_uniform_group_instructions

**Specification**: https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_uniform_group_instructions.html

> **Experimentally supported on**: All platforms

**Capabilities**:

- **GroupUniformArithmeticKHR**

---

