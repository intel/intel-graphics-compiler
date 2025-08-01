/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines API options accepted by scalar compiler.

// IGC API options {{
let Flags = [IGCApiOption] in {

// Backend API options {{

// -cl-fp32-correctly-rounded-divide-sqrt, -ze-fp32-correctly-rounded-divide-sqrt
defm fp32_correctly_rounded_divide_sqrt : CommonFlag<"fp32-correctly-rounded-divide-sqrt">,
  HelpText<"Allows an application to specify that single precision floating-point divide (x/y and 1/x) and sqrt used in the program source are correctly rounded.">;

// -cl-no-subgroup-ifp, -ze-no-subgroup-ifp
defm no_subgroup_ifp : CommonFlag<"no-subgroup-ifp">,
  HelpText<"This indicates that kernels in this program do not require sub-groups to make independent forward progress.">;

// -cl-uniform-work-group-size, -ze-uniform-work-group-size
defm uniform_work_group_size : CommonFlag<"uniform-work-group-size">,
  HelpText<"This requires that the global work-size be a multiple of the work-group size specified to clEnqueueNDRangeKernel.">;

// -cl-take-global-address, -ze-take-global-address
defm take_global_address : CommonFlag<"take-global-address">;

defm emit_lib_compile_errors : CommonFlag<"emit-lib-compile-errors">;

// -cl-intel-reqd-eu-thread-count
defm required_thread_count : CommonSeparate<"reqd-eu-thread-count">;
defm : CommonJoined<"reqd-eu-thread-count=">, Alias<required_thread_count_common>;

// -ze-opt-large-grf-kernel
defm large_grf_kernel : CommonSeparate<"large-grf-kernel">,
  HelpText<"-ze-opt-large-grf-kernel <string> tells IGC to use large GRF size if kernel name contains <string> regardless of module-level options.">;

// -ze-opt-regular-grf-kernel
defm regular_grf_kernel : CommonSeparate<"regular-grf-kernel">,
  HelpText<"-ze-opt-regular-grf-kernel <string> tells IGC to use regular GRF size if kernel name contains <string >regardless of module-level options.">;

// -cl-intel-no-local-to-generic, -ze-opt-no-local-to-generic
defm no_local_to_generic : CommonFlag<"no-local-to-generic">;

// -cl-intel-128-GRF-per-thread
defm 128_grf_per_thread : CommonFlag<"128-GRF-per-thread">;

// -cl-intel-256-GRF-per-thread
defm 256_grf_per_thread : CommonFlag<"256-GRF-per-thread">;

// -cl-intel-greater-than-4GB-buffer-required, -ze-opt-greater-than-4GB-buffer-required
defm greater_than_4GB_buffer_required : CommonFlag<"greater-than-4GB-buffer-required">,
  HelpText<"When this flag is present, it indicates that any OpenCL buffers can be more than 4GB in size. If it is absent, all buffers are not more than 4GB in size.">;

// -cl-poison-unsupported-fp64-kernels -ze-poison-unsupported-fp64-kernels
defm poison_unsupported_fp64_kernels : CommonFlag<"poison-unsupported-fp64-kernels">;

// -cl-intel-enable-ieee-float-exception-trap
defm enable_ieee_float_exception_trap : CommonFlag<"enable-ieee-float-exception-trap">,
  HelpText<"This flag enables the IEEE exception trap bit in Control Register in the prolog of the kernel.">;

// -cl-fp64-gen-emu  -ze-fp64-gen-emu
defm fp64_gen_emu : CommonFlag<"fp64-gen-emu">;

// -cl-fp64-gen-conv-emu  -ze-fp64-gen-conv-emu
defm fp64_gen_conv_emu : CommonFlag<"fp64-gen-conv-emu">;

// -cl-intel-profile-guided-trimming, -ze-opt-profile-guided-trimming
defm static_profile_guided_trimming : CommonFlag<"static-profile-guided-trimming">,
  HelpText<"Enable static analysis in the kernel trimming.">;

// }} Backend API options

// API options from source translation {{
def cl_std : ClJoined<"std=">,
  HelpText<"Determine the language version to be accepted by the compiler.">;

def single_precision_constant : ClFlag<"single-precision-constant">,
  HelpText<"Forces implicit conversions of double-precision floating-point literals to single precision.">;

def denorms_are_zero : ClFlag<"denorms-are-zero">,
  HelpText<"Controls how single precision and double precision denormalized numbers are handled. If specified as a build option, the single precision denormalized numbers may be flushed to zero; double precision denormalized numbers may also be flushed to zero if the optional extension for double precision is supported">;

def strict_aliasing : ClFlag<"strict-aliasing">,
  HelpText<"Allows the compiler to assume the strictest aliasing rules.">;

def mad_enable : ClFlag<"mad-enable">,
  HelpText<"Allow a * b + c to be replaced by a mad instruction.">;

def no_signed_zeros : ClFlag<"no-signed-zeros">,
  HelpText<"Allow optimizations for floating-point arithmetic that ignore the signedness of zero.">;

def unsafe_math_optimizations : ClFlag<"unsafe-math-optimizations">,
  HelpText<"Allow optimizations for floating-point arithmetic that (a) assume that arguments and results are valid, (b) may violate the IEEE 754 standard, (c) assume relaxed OpenCL numerical compliance requirements as defined in the unsafe math optimization section of the OpenCL C or OpenCL SPIR-V Environment specifications, and (d) may violate edge case behavior in the OpenCL C or OpenCL SPIR-V Environment specifications.">;

def finite_math_only : ClFlag<"finite-math-only">,
  HelpText<"Allow optimizations for floating-point arithmetic that assume that arguments and results are not NaNs, +Inf, -Inf. This option may violate the OpenCL numerical compliance requirements for single precision and double precision floating-point, as well as edge case behavior.">;

def fast_relaxed_math : ClFlag<"fast-relaxed-math">,
  HelpText<"Sets the optimization options -cl-finite-math-only and -cl-unsafe-math-optimizations. This option causes the preprocessor macro __FAST_RELAXED_MATH__ to be defined in the OpenCL program.">;

def match_sincospi : ClFlag<"match-sincospi">;

def oclfe_w : PlainFlag<"w">,
  HelpText<"Remove all warning messages.">;

def oclfe_werror : PlainFlag<"Werror">,
  HelpText<"Treat every warning as an error.">;

def kernel_arg_info : ClFlag<"kernel-arg-info">,
  HelpText<"Allow the compiler to store information about the arguments of a kernel(s) in the program executable. The argument information stored includes the argument name, its type, the address space and access qualifiers used.">;

def oclfe_x : PlainSeparate<"x">,
  HelpText<"Manualy provide type of file. Takes only spir or spir64 as argument.">;

def oclfe_D : PlainJoinedOrSeparate<"D">,
  HelpText<"Manually define macros.">;

def oclfe_I : PlainJoinedOrSeparate<"I">,
  HelpText<"Add directory to the list of directories which will be searched for header files."> ;

def oclfe_spir_std : PlainJoined<"spir-std=">,
  HelpText<"Specify the SPIR version.">;

def oclfe_gline_tables_only : PlainFlag<"gline-tables-only">,
  HelpText<"Generate only line table debug information.">;

def oclfe_triple : PlainSeparate<"triple">;

def oclfe_dwarf_column_info : PlainFlag<"dwarf-column-info">;


def debug_info : ClFlag<"intel-debug-info">;

def disable_a64wa : ClFlag<"intel-disable-a64WA">;
def oclfe_profiler : PlainFlag<"profiler">;

def oclfe_s : PlainSeparate<"s">,
  HelpText<"Strip all symbol table and debug informaton from the output binary.">;

// Additional debug options.
def oclfe_igc_opts : PlainJoinedOrSeparate<"igc_opts">,
  HelpText<"Pass IGC options delimited by ',' or ' '.">;

def oclfe_dump_opt_llvm : PlainJoined<"dump-opt-llvm=">,
  HelpText<"Dump the llvm output to the specified file.">;
// }} API options from source translation

}
// }} IGC API options
