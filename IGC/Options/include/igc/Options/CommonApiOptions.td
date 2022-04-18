/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

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

def large_GRF : ZeFlag<"opt-large-register-file">,
  HelpText<"Increase number of available GRF">;

def emit_debug : PlainFlag<"g">,
  HelpText<"Enable generation of debug information and enables kernel debug">;
// These are coming from NEO when run under debugger.
defm opt_disable : CommonFlag<"opt-disable">,
  HelpText<"Turns off optimizations">;

defm library_compilation : CommonFlag<"library-compilation">;

}
// }} Common API options
