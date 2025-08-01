/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines internal options accepted by scalar compiler.

// IGC internal options {{
let Flags = [IGCInternalOption] in {

// Backend internal options {{

// -cl-replace-global-offsets-by-zero, -ze-replace-global-offsets-by-zero
defm replace_global_offsets_by_zero : CommonFlag<"replace-global-offsets-by-zero">,
  HelpText<"OpenCL's global IDs are assumed to start from the origin at global offsets (offset_x, offset_y, offset_z) When this flag is present, it indicates that the global offsets are (0,0,0).">;

defm kernel_debug_enable : CommonFlag<"kernel-debug-enable">;

defm include_sip_csr : CommonFlag<"include-sip-csr">;

defm include_sip_kernel_debug : CommonFlag<"include-sip-kernel-debug">;

defm include_sip_kernel_local_debug : CommonFlag<"include-sip-kernel-local-debug">;

// -cl-intel-use-32bit-ptr-arith
defm use_32_bit_ptr_arith : CommonFlag<"use-32bit-ptr-arith">;

// -cl-intel-greater-than-4GB-buffer-required, -ze-opt-greater-than-4GB-buffer-required
defm greater_than_4GB_buffer_required : CommonFlag<"greater-than-4GB-buffer-required">,
  HelpText<"When this flag is present, it indicates that any OpenCL buffers can be more than 4GB in size. If it is absent,  all buffers are not more than 4GB in size.">;

// -cl-intel-has-buffer-offset-arg, -ze-opt-has-buffer-offset-arg, -ze-intel-has-buffer-offset-arg
defm has_buffer_offset_arg : CommonFlag<"has-buffer-offset-arg">,
  HelpText<"This flag, together with *[-cl-intel|-ze-opt]-greater-than-4GB-buffer-required* is used to convert stateless memory accesses, called messages or load/store, into stateful ones. The OpenCL runtime can create a surface whose base is either *buffer_base* or *buffer_base + buffer_offset*, based on whether *buffer_offset* is used.">;

// -cl-intel-buffer-offset-arg-required, -ze-opt-buffer-offset-arg-required
defm buffer_offset_arg_required : CommonFlag<"buffer-offset-arg-required">,
  HelpText<"Tell IGC to always use buffer offset. It is valid only if -intel-has-buffer-offset-arg is present.">;

// -cl-intel-has-positive-pointer-offset, -ze-opt-has-positive-pointer-offset
defm has_positive_pointer_offset : CommonFlag<"has-positive-pointer-offset">,
  HelpText<"For any load and store (aka message) whose address = *ptrArg + offset*, where *ptrArg* is a kernel pointer argument, offset is assumed to be non-negative if this flag is present.">;

// -cl-intel-disable-a64WA
defm disable_a64wa : CommonFlag<"disable-a64WA">;

// -cl-intel-force-enable-a64WA
defm force_enable_a64wa : CommonFlag<"force-enable-a64WA">;

// -cl-intel-no-prera-scheduling
defm no_prera_scheduling : CommonFlag<"no-prera-scheduling">;

// (All start with -cl-intel or -ze-opt)
defm num_thread_per_eu : CommonSeparate<"num-thread-per-eu">,
  HelpText<"Overrides the current number of threads value defined by the user's command line option for the entire module or the compiler choice by heuristics.">;

defm exp_register_file_size : CommonSeparate<"exp-register-file-size">,
  HelpText<"Set amount of registers used by regalloc.">;
defm : CommonJoined<"exp-register-file-size=">, Alias<exp_register_file_size_common>,
  HelpText<"Alias for -ze-exp-register-file-size.">;

// -cl-intel-128-GRF-per-thread
defm 128_grf_per_thread : CommonFlag<"128-GRF-per-thread">;

// -cl-intel-256-GRF-per-thread
defm 256_grf_per_thread : CommonFlag<"256-GRF-per-thread">;

// -cl-intel-enable-auto-large-GRF-mode, -ze-opt-enable-auto-large-GRF-mode
defm enable_auto_large_GRF_mode : CommonFlag<"enable-auto-large-GRF-mode">,
  HelpText<"Use compiler heuristics to determine number of GRF.">;

// -cl-intel-force-global-mem-allocation, -cl-force-global-mem-allocation, -ze-force-global-mem-allocation
defm force_global_mem_allocation : CommonFlag<"force-global-mem-allocation">;


// (All start with -cl-intel or -ze-opt)

// -cl-disable-recompilation, -ze-opt-disable-recompilation
defm disable_recompilation : CommonFlag<"disable-recompilation">;

// -cl-intel-force-emu-int32divrem
defm force_emu_int32divrem : CommonFlag<"force-emu-int32divrem">,
  HelpText<"Use emulation fp64-based emulation functions if fp64 is supported natively.">;

// -cl-intel-force-emu-sp-int32divrem
defm force_emu_sp_int32divrem : CommonFlag<"force-emu-sp-int32divrem">,
  HelpText<"Force the fp64-based emulation regardless of native support.">;

// -cl-intel-force-disable-4GB-buffer
defm force_disable_4GB_buffer : CommonFlag<"force-disable-4GB-buffer">;

// -cl-intel-use-bindless-mode
defm use_bindless_mode : CommonFlag<"use-bindless-mode">;

// -cl-intel-use-bindless-printf
defm use_bindless_printf : CommonFlag<"use-bindless-printf">;

// -cl-intel-use-bindless-legacy-mode
defm use_bindless_legacy_mode : CommonFlag<"use-bindless-legacy-mode">;

// -cl-intel-use-bindless-advanced-mode
defm use_bindless_advanced_mode : CommonFlag<"use-bindless-advanced-mode">;

// -cl-intel-vector-coalesing
defm vector_coalescing : CommonSeparate<"vector-coalesing">;
defm : CommonJoined<"vector-coalesing=">, Alias<vector_coalescing_common>;

// -cl-intel-exclude-ir-from-zebin
defm exclude_ir_from_zebin : CommonFlag<"exclude-ir-from-zebin">,
  HelpText<"Exclude SPIR-V section from files generated in ZEBIN format.">;

// -cl-intel-no-spill
defm no_spill : CommonFlag<"no-spill">;

// -cl-intel-disable-noMaskWA, -ze-intel-disable-noMaskWA
defm disable_noMaskWA : CommonFlag<"disable-noMaskWA">;

// -cl-intel-ignoreBFRounding, -ze-intel-ignoreBFRounding
defm ignoreBFRounding : CommonFlag<"ignoreBFRounding">,
  HelpText<"Folds BF operands into mul/add/cmp operations.">;

// -cl-compile-one-at-time
defm compile_one_at_time : CommonFlag<"compile-one-at-time">,
  HelpText<"Enables llvm::module splitting to compile only one kernel at a time.">;

// -cl-skip-reloc-add
defm skip_reloc_add : CommonFlag<"skip-reloc-add">;

// -cl-intel-disableEUFusion -ze-intel-disableEUFusion
defm disableEUFusion : CommonFlag<"disableEUFusion">;

// -cl-intel-functonControl [<n>] -ze-intel-functionControl [<n>]
defm functonControl : CommonSeparate<"functonControl">;

defm fail_on_spill : CommonFlag<"fail-on-spill">;

// -[cl|ze]-load-cache-default[=| ]<positive int>
defm load_cache_default : CommonSeparate<"load-cache-default">;
defm : CommonJoined<"load-cache-default=">, Alias<load_cache_default_common>;

// -[cl|ze]-store-cache-default[=| ]<positive int>
defm store_cache_default : CommonSeparate<"store-cache-default">;
defm : CommonJoined<"store-cache-default=">, Alias<store_cache_default_common>;

// -cl-fp64-gen-emu  -ze-fp64-gen-emu
defm fp64_gen_emu : CommonFlag<"fp64-gen-emu">,
  HelpText<"Enable full FP64 emulation.">;

// *-private-memory-minimal-size-per-thread <SIZE>
defm private_memory_minimal_size_per_thread : CommonSeparate<"private-memory-minimal-size-per-thread">,
  HelpText<"When this flag is present, it guarantees that size of private memory allocated per thread can not be less then the given value. Constraint: <SIZE> >= 0.">;

// *-scratch-space-private-memory-minimal-size-per-thread <SIZE>
defm scratch_space_private_memory_minimal_size_per_thread : CommonSeparate<"scratch-space-private-memory-minimal-size-per-thread">,
  HelpText<"When this flag is present, it guarantees that size of scratch space private memory allocated per thread can not be less then the given value. Constraint: <SIZE> >= 0.">;

defm enable_divergent_barrier_handling : CommonFlag<"enable-divergent-barrier-handling">,
  HelpText<"Enable the divergent barrier pass.">;

// -cl-intel-high-accuracy-nolut-math
defm high_accuracy_nolut_math : CommonFlag<"high-accuracy-nolut-math">,
  HelpText<"Enbales experimental high accuracy implementations of transcendentals.">;

// -[cl-intel|ze-opt]-ldstcombine=[0|1]
// -[cl-intel|ze-opt]-ldstcombine-max-storebytes=[4|8|16|32]
// -[cl-intel|ze-opt]-ldstcombine-max-loadbytes=[4|8|16|32]
defm ldstcombine : CommonSeparate<"ldstcombine">;
defm : CommonJoined<"ldstcombine=">, Alias<ldstcombine_common>;
defm ldstcombine_max_storebytes : CommonSeparate<"ldstcombine_max_storebytes">;
defm : CommonJoined<"ldstcombine-max-storebytes=">, Alias<ldstcombine_max_storebytes_common>;
defm ldstcombine_max_loadbytes : CommonSeparate<"ldstcombine_max-loadbytes">;
defm : CommonJoined<"ldstcombine-max-loadbytes=">, Alias<ldstcombine_max_loadbytes_common>;

// }} Backend internal options

// Internal options from source translation {{
def oclfe_ocl_version : PlainJoined<"ocl-version=">;

def oclfe_force_cl_std : PlainFlag<"force-cl-std">,
  HelpText<"Force a specific OpenCL C version.">;

def oclfe_32bit : PlainFlag<"m32">;

def oclfe_64bit : PlainFlag<"m64">;

def oclfe_D : PlainJoinedOrSeparate<"D">,
  HelpText<"Manually define macros.">;

def oclfe_cl_ext : ClJoined<"ext=">;
// }} Internal options from source translation

}
// }} IGC internal options
