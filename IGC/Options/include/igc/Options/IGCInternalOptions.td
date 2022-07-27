/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines internal options accepted by scalar compiler.

// IGC internal options {{
let Flags = [IGCInternalOption] in {

// Backend internal options {{
defm replace_global_offsets_by_zero :
  CommonFlag<"replace-global-offsets-by-zero">;

defm kernel_debug_enable : CommonFlag<"kernel-debug-enable">;

defm include_sip_csr : CommonFlag<"include-sip-csr">;

defm include_sip_kernel_debug : CommonFlag<"include-sip-kernel-debug">;

defm include_sip_kernel_local_debug :
  CommonFlag<"include-sip-kernel-local-debug">;

defm use_32_bit_ptr_arith : CommonFlag<"intel-use-32bit-ptr-arith">;

def has_buffer_offset_arg : ZeFlag<"opt-has-buffer-offset-arg">;
def : ClFlag<"intel-has-buffer-offset-arg">, Alias<has_buffer_offset_arg>;

def buffer_offset_arg_required : ZeFlag<"opt-buffer-offset-arg-required">;
def : ClFlag<"intel-buffer-offset-arg-required">,
  Alias<buffer_offset_arg_required>;

def has_positive_pointer_offset : ZeFlag<"opt-has-positive-pointer-offset">;
def : ClFlag<"intel-has-positive-pointer-offset">,
  Alias<has_positive_pointer_offset>;

def has_subdw_aligned_ptr_arg : ZeFlag<"opt-has-subDW-aligned-ptr-arg">;
def : ClFlag<"intel-has-subDW-aligned-ptr-arg">,
  Alias<has_subdw_aligned_ptr_arg>;

defm intel_disable_a64wa : CommonFlag<"intel-disable-a64WA">;
defm intel_force_enable_a64wa : CommonFlag<"intel-force-enable-a64WA">;

defm intel_gtpin_rera : CommonFlag<"intel-gtpin-rera">;

defm intel_enable_prera_scheduling : CommonFlag<"intel-no-prera-scheduling">;

defm intel_128_grf_per_thread : CommonFlag<"intel-128-GRF-per-thread">;

defm opt_large_register_file : CommonFlag<"opt-large-register-file">;
defm : CommonFlag<"intel-256-GRF-per-thread">,
  Alias<opt_large_register_file_ze>;

defm intel_num_thread_per_eu : CommonSeparate<"intel-num-thread-per-eu">;


defm intel_use_bindless_images : CommonFlag<"intel-use-bindless-images">;

defm intel_use_bindless_mode : CommonFlag<"intel-use-bindless-mode">;

defm intel_use_bindless_printf : CommonFlag<"intel-use-bindless-printf">;

defm intel_force_global_mem_allocation :
  CommonFlag<"intel-force-global-mem-allocation">;
defm intel_no_local_to_generic : CommonFlag<"intel-no-local-to-generic">;

defm intel_vector_coalescing : CommonJoined<"intel-vector-coalesing=">;

defm allow_zebin : CommonFlag<"allow-zebin">;

defm intel_no_spill : CommonFlag<"intel-no-spill">;
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
// }} IGC internal options
