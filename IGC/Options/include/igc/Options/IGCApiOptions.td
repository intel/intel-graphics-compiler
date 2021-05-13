/*========================== begin_copyright_notice ============================

Copyright (c) 2020-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

// This file defines API options accepted by scalar compiler.
//
//===----------------------------------------------------------------------===//

// IGC API options {{
let Flags = [IGCApiOption] in {

// Backend API options {{
def : ClFlag<"intel-gtpin-rera">, Alias<gtpin_rera>,
  HelpText<"Alias for -ze-gtpin-rera">;

def : ClFlag<"intel-gtpin-grf-info">, Alias<gtpin_grf_info>,
  HelpText<"Alias for -ze-gtpin-grf-info">;

def : ClSeparate<"intel-gtpin-scratch-area-size">,
  Alias<gtpin_scratch_area_size>,
  HelpText<"Alias for -ze-gtpin-scratch-area-size">;
def : ClJoined<"intel-gtpin-scratch-area-size=">,
  Alias<gtpin_scratch_area_size>,
  HelpText<"Alias for -ze-gtpin-scratch-area-size">;

defm fp32_correctly_rounded_divide_sqrt :
  CommonFlag<"fp32-correctly-rounded-divide-sqrt">;

defm no_subgroup_ifp : CommonFlag<"no-subgroup-ifp">;

defm uniform_work_group_size : CommonFlag<"uniform-work-group-size">;

defm take_global_address : CommonFlag<"take-global-address">;

defm library_compilation : CommonFlag<"library-compilation">;
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

def oclfe_dwarf_column_info : PlainSeparate<"dwarf-column-info">;

def no_prera_scheduling : ClFlag<"intel-no-prera-scheduling">;

def greater_than_4gb_buffer_required :
  ClFlag<"intel-greater-than-4GB-buffer-required">;
def : ZeFlag<"opt-greater-than-4GB-buffer-required">,
  Alias<greater_than_4gb_buffer_required>;

def has_buffer_offset_arg : ClFlag<"intel-has-buffer-offset-arg">;
def : ZeFlag<"opt-has-buffer-offset-arg">, Alias<has_buffer_offset_arg>;

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
