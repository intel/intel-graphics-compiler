/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines internal options accepted by scalar compiler.

// Scalar Compiler internal options {{

// Backend internal options {{
let Flags = [ScalarCompilerInternalOption] in {

defm replace_global_offsets_by_zero :
  CommonFlag<"replace-global-offsets-by-zero">,
  HelpText<"Lack of description">;

defm kernel_debug_enable :
  CommonFlag<"kernel-debug-enable">,
  HelpText<"Lack of description">;

defm include_sip_csr :
  CommonFlag<"include-sip-csr">,
  HelpText<"Lack of description">;

defm include_sip_kernel_debug :
  CommonFlag<"include-sip-kernel-debug">,
  HelpText<"Lack of description">;

defm include_sip_kernel_local_debug :
  CommonFlag<"include-sip-kernel-local-debug">,
  HelpText<"Lack of description">;

defm use_32bit_ptr_arith :
  CommonFlag<"use-32bit-ptr-arith">,
  HelpText<"Lack of description">;

// -cl-intel-greater-than-4GB-buffer-required, -ze-opt-greater-than-4GB-buffer-required
defm greater_than_4GB_buffer_required :
  CommonFlag<"greater-than-4GB-buffer-required">,
  HelpText<"Lack of description">;

// -cl-intel-has-buffer-offset-arg, -ze-opt-has-buffer-offset-arg
defm has_buffer_offset_arg :
  CommonFlag<"has-buffer-offset-arg">,
  HelpText<"Lack of description">;

// -cl-intel-buffer-offset-arg-required, -ze-opt-buffer-offset-arg-required
defm buffer_offset_arg_required :
  CommonFlag<"buffer-offset-arg-required">,
  HelpText<"Lack of description">;

// -cl-intel-has-positive-pointer-offset, -ze-opt-has-positive-pointer-offset
defm has_positive_pointer_offset :
  CommonFlag<"has-positive-pointer-offset">,
  HelpText<"Lack of description">;

// -cl-intel-has-subDW-aligned-ptr-arg, -ze-opt-has-subDW-aligned-ptr-arg
defm has_subdw_aligned_ptr_arg :
  CommonFlag<"has-subDW-aligned-ptr-arg">,
  HelpText<"Lack of description">;

// -cl-intel-disable-a64WA
defm disable_a64wa :
  CommonFlag<"disable-a64WA">,
  HelpText<"Lack of description">;

// -cl-intel-force-enable-a64WA
defm force_enable_a64wa :
  CommonFlag<"force-enable-a64WA">,
  HelpText<"Lack of description">;

// GTPin flags used by L0 driver runtime
// -cl-intel-gtpin-rera
defm gtpin_rera :
  CommonFlag<"gtpin-rera">,
  HelpText<"Lack of description">;

defm gtpin_grf_info :
  CommonFlag<"gtpin-grf-info">,
  HelpText<"Lack of description">;

defm gtpin_scratch_area_size :
  CommonSeparate<"gtpin-scratch-area-size">,
  HelpText<"Lack of description">;

defm gtpin_indir_ref :
  CommonFlag<"gtpin-indir-ref">,
  HelpText<"Lack of description">;

// -cl-intel-no-prera-scheduling
defm no_prera_scheduling :
  CommonFlag<"no-prera-scheduling">,
  HelpText<"Lack of description">;

// -cl-intel-no-local-to-generic
defm no_local_to_generic :
  CommonFlag<"no-local-to-generic">,
  HelpText<"Lack of description">;

// -cl-intel-force-global-mem-allocation
defm force_global_mem_allocation :
  CommonFlag<"force-global-mem-allocation">,
  HelpText<"Lack of description">;


// Options to set the number of GRF and threads
// (All start with -cl-intel or -ze-opt)
defm 128_grf_per_thread :
 CommonFlag<"128-GRF-per-thread">,
  HelpText<"Lack of description">;

defm 256_grf_per_thread :
 CommonFlag<"256-GRF-per-thread">,
  HelpText<"Lack of description">;

defm large_register_file :
  CommonFlag<"large-register-file">,
  HelpText<"Lack of description">;

defm num_thread_per_eu :
  CommonSeparate<"num-thread-per-eu">,
  HelpText<"Lack of description">;

defm exp_register_file_size :
  CommonSeparate<"exp-register-file-size">,
  HelpText<"Lack of description">;

defm large_grf_kernel :
  CommonSeparate<"large-grf-kernel">,
  HelpText<"Lack of description">;

defm regular_grf_kernel :
  CommonSeparate<"regular-grf-kernel">,
  HelpText<"Lack of description">;

// -cl-disable-recompilation
defm disable_recompilation :
  CommonFlag<"disable-recompilation">,
  HelpText<"Lack of description">;

// -cl-intel-force-emu-int32divrem
defm force_emu_int32divrem :
  CommonFlag<"force-emu-int32divrem">,
  HelpText<"Lack of description">;

// -cl-intel-force-emu-sp-int32divrem
defm force_emu_sp_int32divrem :
  CommonFlag<"force-emu-sp-int32divrem">,
  HelpText<"Lack of description">;

// -cl-intel-force-disable-4GB-buffer
defm force_disable_4GB_buffer :
  CommonFlag<"force-disable-4GB-buffer">,
  HelpText<"Lack of description">;

// This option is in CommonInternalOptions.td
// -cl-intel-use-bindless-buffers
//defm use_bindless_buffers :
//  CommonFlag<"use-bindless-buffers">,
//  HelpText<"Lack of description">;

// -cl-intel-use-bindless-images
defm use_bindless_images :
  CommonFlag<"use-bindless-images">,
  HelpText<"Lack of description">;

// -cl-intel-use-bindless-mode
defm use_bindless_mode :
  CommonFlag<"use-bindless-mode">,
  HelpText<"Lack of description">;

// -cl-intel-use-bindless-printf
defm use_bindless_printf :
  CommonFlag<"use-bindless-printf">,
  HelpText<"Lack of description">;

// -cl-intel-use-bindless-legacy-mode
defm use_bindless_legacy_mode :
  CommonFlag<"use-bindless-legacy-mode">,
  HelpText<"Lack of description">;

// -cl-intel-use-bindless-advanced-mode
defm use_bindless_advanced_mode :
  CommonFlag<"use-bindless-advanced-mode">,
  HelpText<"Lack of description">;

// -cl-intel-vector-coalesing
defm vector_coalescing :
  CommonJoined<"vector-coalesing=">,
  HelpText<"Lack of description">;

// -cl-intel-exclude-ir-from-zebin
defm exclude_ir_from_zebin :
  CommonFlag<"exclude-ir-from-zebin">,
  HelpText<"Lack of description">;

defm emit_zebin_visa_sections :
  CommonFlag<"emit-zebin-visa-sections">,
  HelpText<"Lack of description">;

// -cl-intel-no-spill
defm no_spill :
  CommonFlag<"no-spill">,
  HelpText<"Lack of description">;

// -cl-intel-disable-noMaskWA, -ze-intel-disable-noMaskWA
defm disable_noMaskWA :
  CommonFlag<"disable-noMaskWA">,
  HelpText<"Lack of description">;

// -cl-intel-ignoreBFRounding, -ze-intel-ignoreBFRounding
defm ignoreBFRounding :
  CommonFlag<"ignoreBFRounding">,
  HelpText<"Lack of description">;

// -cl-compile-one-at-time
defm compile_one_at_time :
  CommonFlag<"compile-one-at-time">,
  HelpText<"Lack of description">;

// -cl-skip-reloc-add
defm skip_reloc_add :
  CommonFlag<"skip-reloc-add">,
  HelpText<"Lack of description">;

// -cl-intel-disableEUFusion -ze-intel-disableEUFusion
defm disableEUFusion :
  CommonFlag<"disableEUFusion">,
  HelpText<"Lack of description">;

// -cl-intel-functonControl [<n>] -ze-intel-functionControl [<n>]
defm functonControl :
  CommonSeparate<"functonControl">,
  HelpText<"Lack of description">;

defm fail_on_spill :
  CommonFlag<"fail-on-spill">,
  HelpText<"Lack of description">;

// -[cl|ze]-load-cache-default[=| ]<positive int>
defm load_cache_default :
  CommonSeparate<"load-cache-default">,
  HelpText<"Lack of description">;

// -[cl|ze]-store-cache-default[=| ]<positive int>
defm store_cache_default :
  CommonSeparate<"store-cache-default">,
  HelpText<"Lack of description">;

// -cl-poison-unsupported-fp64-kernels -ze-poison-unsupported-fp64-kernels
defm poison_unsupported_fp64_kernels :
  CommonFlag<"poison-unsupported-fp64-kernels">,
  HelpText<"Lack of description">;

// -cl-fp64-gen-emu  -ze-fp64-gen-emu
defm fp64_gen_emu :
  CommonFlag<"fp64-gen-emu">,
  HelpText<"Lack of description">;

// *-private-memory-minimal-size-per-thread <SIZE>
defm private_memory_minimal_size_per_thread :
  CommonSeparate<"private-memory-minimal-size-per-thread">,
  HelpText<"Lack of description">;

// *-scratch-space-private-memory-minimal-size-per-thread <SIZE>
defm scratch_space_private_memory_minimal_size_per_thread :
  CommonSeparate<"scratch-space-private-memory-minimal-size-per-thread">,
  HelpText<"Lack of description">;

defm enable_divergent_barrier_handling :
  CommonFlag<"enable-divergent-barrier-handling">,
  HelpText<"Lack of description">;

// -cl-intel-high-accuracy-nolut-math
defm high_accuracy_nolut_math :
  CommonFlag<"high-accuracy-nolut-math">,
  HelpText<"Lack of description">;

// }} Backend internal options

// Internal options from source translation {{
def oclfe_ocl_version : PlainJoined<"ocl-version=">;

def oclfe_force_cl_std : PlainFlag<"force-cl-std">;

def oclfe_32bit : PlainFlag<"m32">;

def oclfe_64bit : PlainFlag<"m64">;

def oclfe_D : PlainJoinedOrSeparate<"D">;

def oclfe_cl_ext : ClJoined<"ext=">;
// }} Internal options from source translation

}
// }} Scalar Compiler internal options
