/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines the common options accepted both by scalar and vector
// compiler.

// Common API options {{
let Flags = ApiHelper.CommonApiFlags in {

// -cl-intel-gtpin-rera, -ze-gtpin-rera
defm gtpin_rera : CommonFlag<"gtpin-rera">,
  HelpText<"Enables vISA re_ra interface for gtpin structure generation">;

// -cl-intel-gtpin-grf-info, -ze-gtpin-grf-info
defm gtpin_grf_info : CommonFlag<"gtpin-grf-info">,
  HelpText<"Enable vISA grf-info interface for gtpin structure generation">;

// -cl-intel-gtpin-scratch-area-size, -ze-gtpin-scratch-area-sizegtpin-scratch-area-size
defm gtpin_scratch_area_size : CommonSeparate<"gtpin-scratch-area-size">,
  HelpText<"Set gtpin scratch area size">;
defm : CommonJoined<"gtpin-scratch-area-size=">, Alias<gtpin_scratch_area_size_common>,
  HelpText<"Alias for -ze-gtpin-scratch-area-size">;

defm gtpin_indir_ref : CommonFlag<"gtpin-indir-ref">;

defm skip_fde : CommonFlag<"skip-fde">;

def emit_debug : PlainFlag<"g">,
  HelpText<"Enable generation of debug information and enables kernel debug">;
// These are coming from NEO when run under debugger.
defm opt_disable : CommonFlag<"opt-disable">,
  HelpText<"Turns off optimizations">;

// -library-compilation
defm library_compilation : CommonFlag<"library-compilation">;

// -library-compile-simd=[8|16|32]
defm library_compile_simd : CommonSeparate<"library-compile-simd">;
defm : CommonJoined<"library-compile-simd=">, Alias<library_compile_simd_common>;

// -cl-enable-zebin, -ze-enable-zebin
defm enable_zebin : CommonFlag<"enable-zebin">,
  HelpText<"Emit level zero binary (zebin)">;

// -cl-disable-zebin, -ze-disable-zebin
defm disable_zebin : CommonFlag<"disable-zebin">,
  HelpText<"Emit binary in legacy format">;

defm exp_register_file_size : CommonSeparate<"exp-register-file-size">,
  HelpText<"Set amount of registers used by regalloc">;
defm : CommonJoined<"exp-register-file-size=">, Alias<exp_register_file_size_common>,
  HelpText<"Alias for -ze-exp-register-file-size">;

// -ze-opt-large-register-file
defm large_register_file : CommonFlag<"large-register-file">,
  Alias<exp_register_file_size_common>, AliasArgs<["256"]>,
  HelpText<"Increase number of available GRF">;

// -cl-intel-enable-auto-large-GRF-mode, -ze-opt-enable-auto-large-GRF-mode
defm enable_auto_large_GRF_mode : CommonFlag<"enable-auto-large-GRF-mode">,
  HelpText<"Use compiler heuristics to determine number of GRF">;

}
// }} Common API options
