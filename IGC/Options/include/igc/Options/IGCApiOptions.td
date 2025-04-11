/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines API options accepted by scalar compiler.

// IGC API options {{
let Flags = [IGCApiOption] in {

// Backend API options {{

// -cl-fp32-correctly-rounded-divide-sqrt, -ze-fp32-correctly-rounded-divide-sqrt
defm fp32_correctly_rounded_divide_sqrt : CommonFlag<"fp32-correctly-rounded-divide-sqrt">;

// -cl-no-subgroup-ifp, -ze-no-subgroup-ifp
defm no_subgroup_ifp : CommonFlag<"no-subgroup-ifp">;

// -cl-uniform-work-group-size, -ze-uniform-work-group-size
defm uniform_work_group_size : CommonFlag<"uniform-work-group-size">;

// -cl-take-global-address, -ze-take-global-address
defm take_global_address : CommonFlag<"take-global-address">;

defm emit_lib_compile_errors : CommonFlag<"emit-lib-compile-errors">;

// -cl-intel-reqd-eu-thread-count
defm required_thread_count : CommonSeparate<"reqd-eu-thread-count">;
defm : CommonJoined<"reqd-eu-thread-count=">, Alias<required_thread_count_common>;

// -ze-opt-large-grf-kernel
defm large_grf_kernel : CommonSeparate<"large-grf-kernel">;

// -ze-opt-regular-grf-kernel
defm regular_grf_kernel : CommonSeparate<"regular-grf-kernel">;

// -cl-intel-no-local-to-generic, -ze-opt-no-local-to-generic
defm no_local_to_generic : CommonFlag<"no-local-to-generic">;

// -cl-intel-128-GRF-per-thread
defm 128_grf_per_thread : CommonFlag<"128-GRF-per-thread">;

// -cl-intel-256-GRF-per-thread
defm 256_grf_per_thread : CommonFlag<"256-GRF-per-thread">;

// -cl-intel-greater-than-4GB-buffer-required, -ze-opt-greater-than-4GB-buffer-required
defm greater_than_4GB_buffer_required : CommonFlag<"greater-than-4GB-buffer-required">;

// -cl-poison-unsupported-fp64-kernels -ze-poison-unsupported-fp64-kernels
defm poison_unsupported_fp64_kernels : CommonFlag<"poison-unsupported-fp64-kernels">;

// -cl-intel-enable-ieee-float-exception-trap
// This flags enables the IEEE exception trap bit in Control Register in the prolog of the kernel.
defm enable_ieee_float_exception_trap : CommonFlag<"enable-ieee-float-exception-trap">;

// -cl-fp64-gen-emu  -ze-fp64-gen-emu
defm fp64_gen_emu : CommonFlag<"fp64-gen-emu">;

// -cl-fp64-gen-conv-emu  -ze-fp64-gen-conv-emu
defm fp64_gen_conv_emu : CommonFlag<"fp64-gen-conv-emu">;

// -cl-intel-profile-guided-trimming, -ze-opt-profile-guided-trimming
defm static_profile_guided_trimming : CommonFlag<"static-profile-guided-trimming">;

// }} Backend API options

// API options from source translation {{
def cl_std : ClJoined<"std=">;

def single_precision_constant : ClFlag<"single-precision-constant">;

def denorms_are_zero : ClFlag<"denorms-are-zero">;

def strict_aliasing : ClFlag<"strict-aliasing">;

def mad_enable : ClFlag<"mad-enable">;

def no_signed_zeros : ClFlag<"no-signed-zeros">;

def unsafe_math_optimizations : ClFlag<"unsafe-math-optimizations">;

def finite_math_only : ClFlag<"finite-math-only">;

def fast_relaxed_math : ClFlag<"fast-relaxed-math">;

def match_sincospi : ClFlag<"match-sincospi">;

def oclfe_w : PlainFlag<"w">;

def oclfe_werror : PlainFlag<"Werror">;

def kernel_arg_info : ClFlag<"kernel-arg-info">;

def oclfe_x : PlainSeparate<"x">;

def oclfe_D : PlainJoinedOrSeparate<"D">;

def oclfe_I : PlainJoinedOrSeparate<"I">;

def oclfe_spir_std : PlainJoined<"spir-std=">;

def oclfe_gline_tables_only : PlainFlag<"gline-tables-only">;

def oclfe_triple : PlainSeparate<"triple">;

def oclfe_dwarf_column_info : PlainFlag<"dwarf-column-info">;


def debug_info : ClFlag<"intel-debug-info">;

def disable_a64wa : ClFlag<"intel-disable-a64WA">;

def oclfe_profiler : PlainFlag<"profiler">;

def oclfe_s : PlainSeparate<"s">;

// Additional debug options.
def oclfe_igc_opts : PlainJoinedOrSeparate<"igc_opts">;

def oclfe_dump_opt_llvm : PlainJoined<"dump-opt-llvm=">;
// }} API options from source translation

}
// }} IGC API options
