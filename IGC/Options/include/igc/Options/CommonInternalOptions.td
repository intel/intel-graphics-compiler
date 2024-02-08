/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines the common options accepted both by scalar and vector
// compiler.

// Common internal options {{
let Flags = InternalHelper.CommonInternalFlags in {


// -cl-intel-use-bindless-buffers
defm use_bindless_buffers : CommonFlag<"use-bindless-buffers">,
  HelpText<"Use bindless mode for buffers">;

defm emit_zebin_visa_sections : CommonFlag<"emit-zebin-visa-sections">,
  HelpText<"Add vISA asm as sections in ZeBin">;

def emit_visa_only : PlainFlag<"emit-visa-only">,
  HelpText<"Compile until vISA">;

// -cl-allow-zebin, -ze-allow-zebin
defm allow_zebin : CommonFlag<"allow-zebin">,
  HelpText<"Emit level zero binary">;

// -cl-disable-zebin, -ze-disable-zebin
defm disable_zebin : CommonFlag<"disable-zebin">,
  HelpText<"Emit binary in legacy format">;

defm buffer_bounds_checking : CommonFlag<"buffer-bounds-checking">,
  HelpText<"Enable buffer bounds checking">;

}
// }} Common internal options
