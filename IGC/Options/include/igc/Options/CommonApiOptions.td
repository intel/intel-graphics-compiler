/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines the common options accepted both by scalar and vector
// compiler.

// Common API options {{
let Flags = ApiHelper.CommonApiFlags in {

def gtpin_rera : ZeFlag<"gtpin-rera">,
  HelpText<"Enables vISA re_ra interface for gtpin structure generation">;

def gtpin_grf_info : ZeFlag<"gtpin-grf-info">,
  HelpText<"Enable vISA grf-info interface for gtpin structure generation">;

def gtpin_scratch_area_size : ZeSeparate<"gtpin-scratch-area-size">,
  HelpText<"Set gtpin scratch area size">;
def : ZeJoined<"gtpin-scratch-area-size=">, Alias<gtpin_scratch_area_size>,
  HelpText<"Alias for -ze-gtpin-scratch-area-size">;

def emit_debug : PlainFlag<"g">,
  HelpText<"Enable generation of debug information and enables kernel debug">;
// These are coming from NEO when run under debugger.
defm opt_disable : CommonFlag<"opt-disable">,
  HelpText<"Turns off optimizations">;

defm library_compilation : CommonFlag<"library-compilation">;

defm enable_zebin : CommonFlag<"enable-zebin">,
  HelpText<"Emit level zero binary (zebin)">;
def intel_enable_zebin : ClFlag<"intel-enable-zebin">,
  Alias<enable_zebin_ze>,
  HelpText<"Alias for -cl-enable-zebin">;

defm disable_zebin : CommonFlag<"disable-zebin">,
  HelpText<"Emit binary in legacy format">;
def intel_disable_zebin : ClFlag<"intel-disable-zebin">,
  Alias<disable_zebin_ze>,
  HelpText<"Alias for -cl-disable-zebin">;

def register_file_size : ZeSeparate<"exp-register-file-size">,
  HelpText<"Set amount of registers used by regalloc">;
def : ZeJoined<"exp-register-file-size=">, Alias<register_file_size>,
  HelpText<"Alias for -ze-exp-register-file-size">;

def large_GRF : ZeFlag<"opt-large-register-file">,
  Alias<register_file_size>, AliasArgs<["256"]>,
  HelpText<"Increase number of available GRF">;

defm intel_enable_auto_large_GRF_mode : CommonFlag<"intel-enable-auto-large-GRF-mode">,
  HelpText<"Use compiler heuristics to determine number of GRF">;
}
// }} Common API options
