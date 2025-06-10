/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines internal options accepted by vector compiler.

// VC internal options {{
let Flags = [VCInternalOption] in {

def dump_llvm_ir : PlainFlag<"dump-llvm-ir">,
  HelpText<"Dump llvm IR after SPIRV reading, optimizations and codegen">;
def dump_isa_binary : PlainFlag<"dump-isa-binary">,
  HelpText<"Dump isa binary after finalization pass">;
def dump_asm : PlainFlag<"dump-asm">,
  HelpText<"Dump assembly (visaasm, asm, etc.)">;
def : PlainFlag<"mdump_asm">, Alias<dump_asm>,
  HelpText<"Alias for -dump-asm">;

def help : PlainFlag<"help">,
  HelpText<"Display available API options">;
def help_internal : PlainFlag<"help-internal">,
  HelpText<"Display available internal options">;

def llvm_options : PlainSeparate<"llvm-options">,
  HelpText<"Additional options forwarded to llvm CommandLine global option parser">;
def : PlainJoined<"llvm-options=">, Alias<llvm_options>,
  HelpText<"Alias for -llvm-options">;

def target_features : PlainSeparate<"target-features">,
  HelpText<"Auxiliary target features">;
def : PlainJoined<"target-features=">, Alias<target_features>,
  HelpText<"Alias for -target-features">;

def binary_format : PlainSeparate<"binary-format">,
  HelpText<"Set in which format should be generated binary; values: 'cm', 'ocl' or 'ze'">;
def : PlainJoined<"binary-format=">, Alias<binary_format>,
  HelpText<"Alias for -binary-format <value>">;

def : PlainSeparate<"runtime">, Alias<binary_format>,
  HelpText<"Set runtime for which binary should be generated; values: 'ocl' or 'cm'">;
def : PlainJoined<"runtime=">, Alias<binary_format>,
  HelpText<"Alias for -runtime <value>">;

def ftime_report : PlainFlag<"ftime-report">,
  HelpText<"Print timing summary of each stage of compilation">;
def freset_time_report : PlainFlag<"freset-time-report">,
  HelpText<"Reset timing summary before compilation">;

def print_stats : PlainFlag<"print-stats">,
  HelpText<"Print performance metrics and statistics">;
def freset_llvm_stats : PlainFlag<"freset-llvm-stats">,
  HelpText<"Reset performance metrics before compilation">;

def stats_file : PlainSeparate<"stats-file">,
  HelpText<"Filename to write statistics to">;
def : PlainJoined<"stats-file=">, Alias<stats_file>,
  HelpText<"Alias for -stats-file">;

def fdisable_debuggable_kernels : PlainFlag<"vc-disable-debuggable-kernels">,
  HelpText<"Disable emission of debuggable kernels for legacy path">;

def gpu_scope_fence : PlainFlag<"gpu-scope-fence-on-single-tile">,
  HelpText<"Allow the use of \"GPU\" fence scope on single-tile GPUs">;
def flush_l3_for_global : PlainFlag<"flush-l3-for-global">,
  HelpText<"Enable flushing L3 cache for globals">;

def vc_loop_unroll_threshold : PlainSeparate<"vc-loop-unroll-threshold">,
  HelpText<"Loop unroll threshold for VC">;
def : PlainJoined<"vc-loop-unroll-threshold=">, Alias<vc_loop_unroll_threshold>,
  HelpText<"Alias for -vc-loop-unroll-threshold <value>">;

def vc_ignore_loop_unroll_threshold_on_pragma : PlainFlag<"vc-ignore-loop-unroll-threshold-on-pragma">,
  HelpText<"Ignore loop unroll threshold on pragma for VC">;

def vc_interop_subgroup_size : PlainSeparate<"vc-interop-subgroup-size">,
  HelpText<"Set subgroup size used for cross-module calls/returns">;

def vc_report_lsc_stores_with_non_default_l1_cache_controls :
  PlainFlag<"vc-report-lsc-stores-with-non-default-l1-cache-controls">,
  HelpText<"Ignore loop unroll threshold on pragma for VC">;
}
// }} VC internal options
