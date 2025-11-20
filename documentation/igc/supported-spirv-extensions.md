# Supported SPIR-V Extensions

This document lists all SPIR-V extensions supported by IGC and their platform requirements.

## SPV_EXT_float8

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_float8.html

**Extension Platform Support:** IGFX_XE3P_CORE and newer

**Capabilities:**

- **Float8EXT**
- **Float8CooperativeMatrixEXT**

---

## SPV_EXT_optnone

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_optnone.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **OptNoneEXT**

---

## SPV_EXT_shader_atomic_float16_add

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_shader_atomic_float16_add.html

**Extension Platform Support:** IGFX_XE3P_CORE and newer

**Capabilities:**

- **AtomicFloat16AddEXT**

---

## SPV_EXT_shader_atomic_float_add

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_shader_atomic_float_add.html

**Capabilities:**

- **AtomicFloat32AddEXT**
  - Platform Support: All platforms
- **AtomicFloat64AddEXT**
  - Platform Support: IGFX_METEORLAKE and newer

---

## SPV_EXT_shader_atomic_float_min_max

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/EXT/SPV_EXT_shader_atomic_float_min_max.html

**Capabilities:**

- **AtomicFloat16MinMaxEXT**
  - Platform Support: IGFX_XE3P_CORE and newer
- **AtomicFloat32MinMaxEXT**
  - Platform Support: All platforms
- **AtomicFloat64MinMaxEXT**
  - Platform Support: IGFX_METEORLAKE and newer

---

## SPV_INTEL_16bit_atomics

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.com/intel/llvm/pull/20009

**Capabilities:**

- **AtomicInt16CompareExchangeINTEL**
  - Platform Support: IGFX_XE3P_CORE and newer
- **Int16AtomicsINTEL**
  - Platform Support: Not supported
- **AtomicBFloat16LoadStoreINTEL**
  - Platform Support: IGFX_XE3P_CORE and newer
- **AtomicBFloat16AddINTEL**
  - Platform Support: IGFX_XE3P_CORE and newer
- **AtomicBFloat16MinMaxINTEL**
  - Platform Support: IGFX_XE3P_CORE and newer

---

## SPV_INTEL_2d_block_io

**Specification:** https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_2d_block_io.asciidoc

**Extension Platform Support:** IGFX_XE_HPC_CORE and newer

**Capabilities:**

- **Subgroup2DBlockIOINTEL**
- **Subgroup2DBlockTransformINTEL**
- **Subgroup2DBlockTransposeINTEL**

---

## SPV_INTEL_bfloat16_arithmetic

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_bfloat16_arithmetic.asciidoc

**Extension Platform Support:** IGFX_XE3P_CORE and newer

**Capabilities:**

- **BFloat16ArithmeticINTEL**

---

## SPV_INTEL_bfloat16_conversion

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_bfloat16_conversion.html

**Extension Platform Support:** IGFX_XE3P_CORE and newer

**Capabilities:**

- **BFloat16ConversionINTEL**

---

## SPV_INTEL_bindless_images

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_bindless_images.html

**Extension Platform Support:** IGFX_XE_HPG_CORE and newer (excluding IGFX_PVC, IGFX_CRI)

**Capabilities:**

- **BindlessImagesINTEL**

---

## SPV_INTEL_cache_controls

**Specification:** https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_cache_controls.asciidoc

**Extension Platform Support:** All platforms

**Capabilities:**

- **CacheControlsINTEL**

---

## SPV_INTEL_debug_module

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_debug_module.asciidoc

**Extension Platform Support:** All platforms

**Capabilities:**

- **DebugInfoModuleINTEL**

---

## SPV_INTEL_float4

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.com/intel/llvm/pull/20467

**Extension Platform Support:** IGFX_XE3P_CORE and newer

**Capabilities:**

- **Float4TypeINTEL**
- **Float4CooperativeMatrixINTEL**

---

## SPV_INTEL_fp_conversions

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.com/intel/llvm/pull/20467

**Extension Platform Support:** IGFX_XE3P_CORE and newer

**Capabilities:**

- **FloatConversionsINTEL**

---

## SPV_INTEL_fp_fast_math_mode

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_fp_fast_math_mode.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **FPFastMathModeINTEL**

---

## SPV_INTEL_fp_max_error

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_fp_max_error.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **FPMaxErrorINTEL**

---

## SPV_INTEL_function_pointers

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_function_pointers.asciidoc

**Extension Platform Support:** All platforms

**Capabilities:**

- **FunctionPointersINTEL**
- **IndirectReferencesINTEL**

---

## SPV_INTEL_global_variable_host_access

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_global_variable_host_access.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **GlobalVariableHostAccessINTEL**

---

## SPV_INTEL_inline_assembly

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_inline_assembly.asciidoc

**Extension Platform Support:** All platforms

**Capabilities:**

- **AsmINTEL**

---

## SPV_INTEL_int4

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/INTEL/SPV_INTEL_int4.asciidoc

**Capabilities:**

- **Int4TypeINTEL**
  - Platform Support: IGFX_XE3P_CORE and newer
- **Int4CooperativeMatrixINTEL**
  - Platform Support: Not supported

---

## SPV_INTEL_joint_matrix

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_joint_matrix.asciidoc

**Capabilities:**

- **PackedCooperativeMatrixINTEL**
  - Platform Support: IGFX_XE_HPG_CORE and newer
- **CooperativeMatrixInvocationInstructionsINTEL**
  - Platform Support: IGFX_XE_HPG_CORE and newer
- **CooperativeMatrixTF32ComponentTypeINTEL**
  - Platform Support: IGFX_XE_HPC_CORE and newer
- **CooperativeMatrixBFloat16ComponentTypeINTEL**
  - Platform Support: IGFX_XE_HPG_CORE and newer
- **CooperativeMatrixPrefetchINTEL**
  - Platform Support: IGFX_XE_HPC_CORE and newer

---

## SPV_INTEL_kernel_attributes

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_kernel_attributes.html

**Capabilities:**

- **KernelAttributesINTEL**
  - Platform Support: All platforms
- **FPGAKernelAttributesINTEL**
  - Platform Support: Not supported
- **FPGAKernelAttributesv2INTEL**
  - Platform Support: Not supported

---

## SPV_INTEL_long_composites

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_long_composites.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **CapabilityLongCompositesINTEL**

---

## SPV_INTEL_memory_access_aliasing

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.com/intel/llvm/pull/3426/files

**Extension Platform Support:** All platforms

**Capabilities:**

- **MemoryAccessAliasingINTEL**

---

## SPV_INTEL_predicated_io

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_predicated_io.asciidoc

**Extension Platform Support:** All platforms

**Capabilities:**

- **PredicatedIOINTEL**

---

## SPV_INTEL_sigmoid

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_sigmoid.asciidoc

**Extension Platform Support:** All platforms

**Capabilities:**

- **SigmoidINTEL**

---

## SPV_INTEL_split_barrier

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_split_barrier.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **SplitBarrierINTEL**

---

## SPV_INTEL_subgroup_buffer_prefetch

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_subgroup_buffer_prefetch.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **SubgroupBufferPrefetchINTEL**

---

## SPV_INTEL_subgroup_matrix_multiply_accumulate

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_subgroup_matrix_multiply_accumulate.html

**Extension Platform Support:** IGFX_XE_HPG_CORE and newer (excluding IGFX_METEORLAKE, IGFX_ARROWLAKE)

**Capabilities:**

- **SubgroupMatrixMultiplyAccumulateINTEL**

---

## SPV_INTEL_subgroups

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_subgroups.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **SubgroupShuffleINTEL**
- **SubgroupBufferBlockIOINTEL**
- **SubgroupImageBlockIOINTEL**

---

## SPV_INTEL_tensor_float32_conversion

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_tensor_float32_conversion.html

**Extension Platform Support:** IGFX_XE2_HPG_CORE and newer

**Capabilities:**

- **TensorFloat32RoundingINTEL**

---

## SPV_INTEL_unstructured_loop_controls

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_unstructured_loop_controls.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **UnstructuredLoopControlsINTEL**

---

## SPV_INTEL_variable_length_array

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_variable_length_array.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **VariableLengthArrayINTEL**
- **UntypedVariableLengthArrayINTEL**

---

## SPV_KHR_bfloat16

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_bfloat16.html

**Extension Platform Support:** IGFX_XE2_HPG_CORE and newer

**Capabilities:**

- **BFloat16TypeKHR**
- **BFloat16DotProductKHR**
- **BFloat16CooperativeMatrixKHR**

---

## SPV_KHR_bit_instructions

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_bit_instructions.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **BitInstructions**

---

## SPV_KHR_expect_assume

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_expect_assume.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **ExpectAssumeKHR**

---

## SPV_KHR_integer_dot_product

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_integer_dot_product.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **DotProductKHR**
- **DotProductInputAllKHR**
- **DotProductInput4x8BitKHR**
- **DotProductInput4x8BitPackedKHR**

---

## SPV_KHR_linkonce_odr

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_linkonce_odr.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **Linkage**

---

## SPV_KHR_no_integer_wrap_decoration

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_no_integer_wrap_decoration.html

**Extension Platform Support:** All platforms

**Capabilities:**

- No capabilities defined

---

## SPV_KHR_shader_clock

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_shader_clock.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **ShaderClockKHR**

---

## SPV_KHR_uniform_group_instructions

> **Note**: The support for this extension is experimental. It has been implemented but has not been thoroughly tested and should not be used in production environments.

**Specification:** https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_uniform_group_instructions.html

**Extension Platform Support:** All platforms

**Capabilities:**

- **GroupUniformArithmeticKHR**

---

