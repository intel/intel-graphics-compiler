/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines the common options accepted both by scalar and vector
// compiler.

// Common internal options {{
let Flags = InternalHelper.CommonInternalFlags in {

defm intel_use_bindless_buffers : CommonFlag<"intel-use-bindless-buffers">,
  HelpText<"Use bindless mode for buffers">;

def emit_zebin_visa_sections : PlainFlag<"emit-zebin-visa-sections">,
  HelpText<"Add vISA asm as sections in ZeBin">;

def greater_than_4GB_buffer_required :
  ZeFlag<"opt-greater-than-4GB-buffer-required">;
def : ClFlag<"intel-greater-than-4GB-buffer-required">,
  Alias<greater_than_4GB_buffer_required>;

}
// }} Common internal options
