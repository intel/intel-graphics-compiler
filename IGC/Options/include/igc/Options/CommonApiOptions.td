/*========================== begin_copyright_notice ============================

Copyright (c) 2021-2021 Intel Corporation

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

// This file defines the common options accepted both by scalar and vector
// compiler.
//
//===----------------------------------------------------------------------===//

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
defm : CommonFlag<"opt-disable">,
  Alias<emit_debug>, HelpText<"Alias for -g">;

}
// }} Common API options
